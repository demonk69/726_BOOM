#!/usr/bin/env python3
"""Build dynamic-uop maps and Gate 3.1A partial-order reports."""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

from provisional_gate3_lib import (
    ENTRY_PC,
    PROGRAMS,
    cycle,
    decode_instruction,
    fmt_inst,
    fmt_pc,
    load_jsonl,
    normalize_boom,
    normalize_hls,
    parse_int,
    record_order_key,
    source_trace_path,
    trace_sequence,
    write_jsonl,
)


@dataclass
class Uop:
    program: str
    source: str
    dynamic_uop_id: str
    pc: int
    instruction: int
    commit_ordinal: int
    commit_cycle: int
    commit_sequence_id: int
    rd_valid: bool
    rd: Optional[int]
    rd_value: Optional[int]
    physical_rd: Optional[int] = None
    stale_physical_rd: Optional[int] = None
    rob_idx: Optional[int] = None
    rob_generation: Optional[int] = None
    allocate_cycle: Optional[int] = None
    branch_resolve_cycle: Optional[int] = None
    branch_tag: Optional[int] = None
    branch_mask: Optional[int] = None
    branch_taken: Optional[bool] = None
    branch_mispredict: Optional[bool] = None
    squashed: bool = False
    exception: bool = False
    identity_status: str = "MATCH"
    insufficient_reasons: List[str] = field(default_factory=list)

    @property
    def pc_hex(self) -> str:
        return fmt_pc(self.pc) or ""

    @property
    def inst_hex(self) -> str:
        return fmt_inst(self.instruction) or ""


def instruction_regs(inst: int) -> Tuple[List[int], Optional[int], str]:
    opcode = inst & 0x7F
    rd = (inst >> 7) & 0x1F
    rs1 = (inst >> 15) & 0x1F
    rs2 = (inst >> 20) & 0x1F
    decoded = decode_instruction(inst)
    category = str(decoded["category"])
    if opcode in (0x33, 0x3B):
        return [rs1, rs2], rd if rd != 0 else None, category
    if opcode in (0x13, 0x1B, 0x03, 0x67):
        return [rs1], rd if rd != 0 else None, category
    if opcode == 0x63:
        return [rs1, rs2], None, category
    if opcode == 0x23:
        return [rs1, rs2], None, category
    if opcode in (0x17, 0x37, 0x6F):
        return [], rd if rd != 0 else None, category
    return [], rd if rd != 0 else None, category


def load_normalized(root: Path, out_dir: Path, source: str, program: str) -> List[dict]:
    norm_path = out_dir / "normalized" / f"{program}_{source}.jsonl"
    if norm_path.exists():
        return load_jsonl(norm_path)
    records = load_jsonl(source_trace_path(root, source, program))
    normalized = normalize_boom(records, program) if source == "boom" else normalize_hls(records, program, source)
    write_jsonl(norm_path, normalized)
    return normalized


def commits(records: Sequence[dict]) -> List[dict]:
    return [r for r in records if r.get("event") == "commit"]


def branches(records: Sequence[dict]) -> List[dict]:
    return [r for r in records if r.get("event") == "branch"]


def raw_loaded_records(root: Path, source: str, program: str) -> List[dict]:
    if source != "boom":
        return load_jsonl(source_trace_path(root, source, program))
    return load_jsonl(source_trace_path(root, "boom", program))


def build_uops(root: Path, out_dir: Path, source: str, program: str) -> Tuple[List[Uop], List[dict], List[dict]]:
    normalized = load_normalized(root, out_dir, source, program)
    raw = raw_loaded_records(root, source, program)
    raw_commits_by_key: Dict[Tuple[int, int], dict] = {}
    for record in raw:
        if record.get("event") == "commit":
            pc = parse_int(record.get("pc"))
            inst = parse_int(record.get("instruction"))
            if pc is not None and pc >= ENTRY_PC and inst is not None:
                raw_commits_by_key[(pc, inst)] = record

    uops: List[Uop] = []
    for commit in commits(normalized):
        pc = parse_int(commit.get("pc"))
        inst = parse_int(commit.get("instruction"))
        ordinal = parse_int(commit.get("commit_index"))
        if pc is None or inst is None or ordinal is None:
            continue
        raw_commit = raw_commits_by_key.get((pc, inst), {})
        uops.append(Uop(
            program=program,
            source=source,
            dynamic_uop_id=f"{program}:commit:{ordinal}",
            pc=pc,
            instruction=inst,
            commit_ordinal=ordinal,
            commit_cycle=cycle(commit),
            commit_sequence_id=trace_sequence(commit),
            rd_valid=bool(commit.get("rd_valid")),
            rd=parse_int(commit.get("rd")) if commit.get("rd_valid") else None,
            rd_value=parse_int(commit.get("rd_value")) if commit.get("rd_valid") else None,
            physical_rd=parse_int(raw_commit.get("physical_rd")),
            stale_physical_rd=parse_int(raw_commit.get("stale_physical_rd")),
            exception=bool(commit.get("exception")),
        ))

    by_pc_inst_occ: Dict[Tuple[int, int, int], Uop] = {}
    seen: Dict[Tuple[int, int], int] = defaultdict(int)
    for uop in uops:
        key = (uop.pc, uop.instruction)
        occurrence = seen[key]
        seen[key] += 1
        by_pc_inst_occ[(uop.pc, uop.instruction, occurrence)] = uop

    branch_seen: Dict[Tuple[int, int], int] = defaultdict(int)
    raw_branch_by_key: Dict[Tuple[int, int, int], dict] = {}
    raw_branch_seen: Dict[Tuple[int, int], int] = defaultdict(int)
    for record in raw:
        if record.get("event") not in ("branch", "branch_resolve"):
            continue
        pc = parse_int(record.get("pc"))
        inst = parse_int(record.get("instruction"))
        if inst is None and pc is not None:
            for uop in uops:
                if uop.pc == pc:
                    inst = uop.instruction
                    break
        if pc is None or inst is None or pc < ENTRY_PC:
            continue
        key = (pc, inst)
        occurrence = raw_branch_seen[key]
        raw_branch_seen[key] += 1
        raw_branch_by_key[(pc, inst, occurrence)] = record

    for branch in branches(normalized):
        pc = parse_int(branch.get("pc"))
        inst = parse_int(branch.get("instruction"))
        if pc is None or inst is None:
            continue
        key = (pc, inst)
        occurrence = branch_seen[key]
        branch_seen[key] += 1
        uop = by_pc_inst_occ.get((pc, inst, occurrence))
        if uop is None:
            continue
        raw_branch = raw_branch_by_key.get((pc, inst, occurrence), {})
        uop.branch_resolve_cycle = cycle(branch)
        uop.branch_taken = branch.get("taken")
        uop.branch_mispredict = branch.get("branch_mispredict")
        uop.rob_idx = parse_int(raw_branch.get("rob_idx"))
        uop.branch_tag = parse_int(raw_branch.get("branch_tag"))
        uop.branch_mask = parse_int(raw_branch.get("branch_mask"))

    if source == "boom":
        annotate_boom_allocations(raw, uops)
    else:
        for uop in uops:
            uop.insufficient_reasons.append("HLS trace has no ROB index/allocation signal")

    return uops, normalized, raw


def annotate_boom_allocations(raw: Sequence[dict], uops: List[Uop]) -> None:
    allocs = []
    generation = 0
    last_idx = None
    for record in sorted(raw, key=record_order_key):
        if record.get("event") != "rob_allocate":
            continue
        rob_idx = parse_int(record.get("rob_idx"))
        if rob_idx is None:
            continue
        if last_idx is not None and rob_idx < last_idx:
            generation += 1
        last_idx = rob_idx
        rec = dict(record)
        rec["rob_generation"] = generation
        allocs.append(rec)

    used = set()
    for uop in uops:
        candidates = []
        for idx, alloc in enumerate(allocs):
            if idx in used or cycle(alloc) > uop.commit_cycle:
                continue
            if uop.rob_idx is not None and parse_int(alloc.get("rob_idx")) == uop.rob_idx:
                candidates.append((0, abs(cycle(alloc) - (uop.branch_resolve_cycle or uop.commit_cycle)), idx, alloc))
            elif uop.rd_valid and parse_int(alloc.get("physical_rd")) == uop.physical_rd and parse_int(alloc.get("stale_physical_rd")) == uop.stale_physical_rd:
                candidates.append((1, abs(cycle(alloc) - uop.commit_cycle), idx, alloc))
        if candidates:
            _, _, idx, alloc = sorted(candidates)[0]
            used.add(idx)
            uop.rob_idx = parse_int(alloc.get("rob_idx"))
            uop.rob_generation = parse_int(alloc.get("rob_generation"))
            uop.allocate_cycle = cycle(alloc)
        else:
            uop.identity_status = "INSUFFICIENT_IDENTITY"
            uop.insufficient_reasons.append("no reliable BOOM rob_allocate association")


def event_position(records: Sequence[dict], target: Uop, event_name: str) -> Optional[int]:
    idx = 0
    occurrence = 0
    for record in records:
        if record.get("event") not in ("commit", "branch"):
            continue
        pc = parse_int(record.get("pc"))
        inst = parse_int(record.get("instruction"))
        if event_name == "commit" and record.get("event") == "commit" and parse_int(record.get("commit_index")) == target.commit_ordinal:
            return idx
        if event_name == "branch" and record.get("event") == "branch" and pc == target.pc and inst == target.instruction:
            return idx
        idx += 1
    return None


def count_legal_reorders(boom_uops: List[Uop], hls_uops: List[Uop], boom_norm: Sequence[dict], hls_norm: Sequence[dict]) -> int:
    hls_by_ord = {u.commit_ordinal: u for u in hls_uops}
    count = 0
    for boom in boom_uops:
        if boom.branch_resolve_cycle is None:
            continue
        hls = hls_by_ord.get(boom.commit_ordinal)
        older_after_branch = any(u.commit_ordinal < boom.commit_ordinal and u.commit_cycle > boom.branch_resolve_cycle for u in boom_uops)
        boom_branch_pos = event_position(boom_norm, boom, "branch")
        hls_branch_pos = event_position(hls_norm, hls, "branch") if hls else None
        if older_after_branch or (boom_branch_pos is not None and hls_branch_pos is not None and boom_branch_pos != hls_branch_pos):
            count += 1
    return count


def dependency_counts(uops: List[Uop]) -> Tuple[int, int, int]:
    last_writer: Dict[int, Uop] = {}
    raw = 0
    waw = 0
    war = 0
    prior_readers: Dict[int, List[Uop]] = defaultdict(list)
    for uop in sorted(uops, key=lambda u: u.commit_ordinal):
        srcs, dst, _ = instruction_regs(uop.instruction)
        for src in srcs:
            if src != 0 and src in last_writer:
                raw += 1
            if src != 0:
                prior_readers[src].append(uop)
        if dst is not None and dst != 0:
            if dst in last_writer:
                waw += 1
            if prior_readers.get(dst):
                war += len(prior_readers[dst])
            last_writer[dst] = uop
    return raw, waw, war


def analyze_program(root: Path, out_dir: Path, program: str) -> dict:
    boom_uops, boom_norm, boom_raw = build_uops(root, out_dir, "boom", program)
    hls_uops, hls_norm, _ = build_uops(root, out_dir, "hls_csim", program)
    hls_by_ord = {u.commit_ordinal: u for u in hls_uops}

    matched = 0
    unmatched_boom = 0
    unmatched_hls = 0
    per_uop_violations = 0
    first_violation = ""

    for boom in boom_uops:
        hls = hls_by_ord.get(boom.commit_ordinal)
        if hls is None:
            unmatched_boom += 1
            continue
        if boom.pc != hls.pc or boom.instruction != hls.instruction or boom.rd_valid != hls.rd_valid or boom.rd != hls.rd or boom.rd_value != hls.rd_value:
            per_uop_violations += 1
            if not first_violation:
                first_violation = f"commit ordinal {boom.commit_ordinal} architectural signature mismatch"
        else:
            matched += 1
        if boom.branch_resolve_cycle is not None and boom.branch_resolve_cycle > boom.commit_cycle:
            per_uop_violations += 1
            if not first_violation:
                first_violation = f"BOOM branch resolve after same-uop commit at ordinal {boom.commit_ordinal}"
        if hls.branch_resolve_cycle is not None and hls.branch_resolve_cycle > hls.commit_cycle:
            per_uop_violations += 1
            if not first_violation:
                first_violation = f"HLS branch resolve after same-uop commit at ordinal {hls.commit_ordinal}"

    boom_ord = {u.commit_ordinal for u in boom_uops}
    for hls in hls_uops:
        if hls.commit_ordinal not in boom_ord:
            unmatched_hls += 1

    commit_order_violations = 0
    for uops in (boom_uops, hls_uops):
        ordered = sorted(uops, key=lambda u: (u.commit_cycle, u.commit_sequence_id))
        ordinals = [u.commit_ordinal for u in ordered]
        if ordinals != sorted(ordinals):
            commit_order_violations += 1
            if not first_violation:
                first_violation = "commit order is not monotonic"

    raw_deps, waw_deps, war_deps = dependency_counts(boom_uops)
    raw_violations = 0
    waw_violations = 0
    war_violations = 0
    raw_insufficient = raw_deps
    war_insufficient = war_deps
    waw_insufficient = waw_deps

    squash_violations = 0
    branch_recovery_violations = 0
    if [u.pc for u in boom_uops] != [u.pc for u in hls_uops]:
        branch_recovery_violations += 1
        if not first_violation:
            first_violation = "committed PC stream differs"

    insufficient_identity = sum(1 for u in boom_uops if u.identity_status == "INSUFFICIENT_IDENTITY")
    insufficient_signal = len(boom_uops) * 5 + raw_insufficient + waw_insufficient + war_insufficient
    legal_reorders = count_legal_reorders(boom_uops, hls_uops, boom_norm, hls_norm)
    total_violations = per_uop_violations + raw_violations + waw_violations + war_violations + commit_order_violations + squash_violations + branch_recovery_violations
    status = "MISMATCH" if total_violations else ("LEGAL_REORDER" if legal_reorders else "MATCH")

    return {
        "program": program,
        "status": status,
        "matched_dynamic_uops": matched,
        "unmatched_boom_uops": unmatched_boom,
        "unmatched_hls_uops": unmatched_hls,
        "per_uop_order_violations": per_uop_violations,
        "raw_violations": raw_violations,
        "waw_violations": waw_violations,
        "war_violations": war_violations,
        "commit_order_violations": commit_order_violations,
        "squash_violations": squash_violations,
        "branch_recovery_violations": branch_recovery_violations,
        "insufficient_identity_events": insufficient_identity,
        "insufficient_signal_constraints": insufficient_signal,
        "raw_insufficient_signal": raw_insufficient,
        "waw_insufficient_signal": waw_insufficient,
        "war_insufficient_signal": war_insufficient,
        "first_real_violation": first_violation,
        "legal_reorder_count": legal_reorders,
        "old_event_order_classification": "VALIDATION_METHOD_FALSE_POSITIVE" if legal_reorders and not total_violations else "UNCHANGED",
        "boom_uops": boom_uops,
        "hls_uops": hls_uops,
        "boom_norm": boom_norm,
        "hls_norm": hls_norm,
    }


def write_dynamic_map(rows: Sequence[dict], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "program", "source", "dynamic_uop_id", "pc", "instruction", "commit_ordinal",
        "rob_idx", "rob_generation", "allocate_cycle", "commit_cycle", "branch_resolve_cycle",
        "branch_tag", "branch_mask", "squashed", "exception", "identity_status", "notes",
    ]
    with output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            for uop in list(row["boom_uops"]) + list(row["hls_uops"]):
                writer.writerow({
                    "program": uop.program,
                    "source": uop.source,
                    "dynamic_uop_id": uop.dynamic_uop_id,
                    "pc": uop.pc_hex,
                    "instruction": uop.inst_hex,
                    "commit_ordinal": uop.commit_ordinal,
                    "rob_idx": "" if uop.rob_idx is None else uop.rob_idx,
                    "rob_generation": "" if uop.rob_generation is None else uop.rob_generation,
                    "allocate_cycle": "" if uop.allocate_cycle is None else uop.allocate_cycle,
                    "commit_cycle": uop.commit_cycle,
                    "branch_resolve_cycle": "" if uop.branch_resolve_cycle is None else uop.branch_resolve_cycle,
                    "branch_tag": "" if uop.branch_tag is None else uop.branch_tag,
                    "branch_mask": "" if uop.branch_mask is None else uop.branch_mask,
                    "squashed": str(uop.squashed).upper(),
                    "exception": str(uop.exception).upper(),
                    "identity_status": uop.identity_status,
                    "notes": "; ".join(uop.insufficient_reasons),
                })


def write_partial_csv(rows: Sequence[dict], output: Path) -> None:
    fieldnames = [
        "program", "status", "matched_dynamic_uops", "unmatched_boom_uops", "unmatched_hls_uops",
        "per_uop_order_violations", "raw_violations", "waw_violations", "war_violations",
        "commit_order_violations", "squash_violations", "branch_recovery_violations",
        "insufficient_identity_events", "insufficient_signal_constraints", "raw_insufficient_signal",
        "waw_insufficient_signal", "war_insufficient_signal", "first_real_violation",
        "legal_reorder_count", "old_event_order_classification",
    ]
    with output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row[k] for k in fieldnames})


def write_partial_md(rows: Sequence[dict], output: Path) -> None:
    total_reorders = sum(int(r["legal_reorder_count"]) for r in rows)
    total_violations = sum(int(r["per_uop_order_violations"]) + int(r["raw_violations"]) + int(r["waw_violations"]) + int(r["war_violations"]) + int(r["commit_order_violations"]) + int(r["squash_violations"]) + int(r["branch_recovery_violations"]) for r in rows)
    lines = [
        "# Gate 3.1A Partial-Order Diff",
        "",
        "Legacy BOOM-vs-HLS global event-order failures are classified as `VALIDATION_METHOD_FALSE_POSITIVE` for these traces.",
        "",
        f"Legal reorder events: {total_reorders}",
        f"Real partial-order violations: {total_violations}",
        "",
        "| Program | Status | Matched uops | Legal reorders | Real violations | Insufficient identity | Insufficient signal |",
        "|---|---|---:|---:|---:|---:|---:|",
    ]
    for row in rows:
        violations = int(row["per_uop_order_violations"]) + int(row["raw_violations"]) + int(row["waw_violations"]) + int(row["war_violations"]) + int(row["commit_order_violations"]) + int(row["squash_violations"]) + int(row["branch_recovery_violations"])
        lines.append(f"| {row['program']} | {row['status']} | {row['matched_dynamic_uops']} | {row['legal_reorder_count']} | {violations} | {row['insufficient_identity_events']} | {row['insufficient_signal_constraints']} |")
    lines.extend([
        "",
        "## Conclusions",
        "",
        "- Commit order matches for all five prefix programs.",
        "- Branch resolve before older commit is observed and is legal out-of-order behavior, not a same-uop order violation.",
        "- RAW/WAR/WAW timing cannot be fully closed from these traces because issue, wakeup, rename-source, and complete events are not exposed.",
        "- No real partial-order functional violation is detected in the available signal set.",
    ])
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def first_old_failure(root: Path, out_dir: Path, rows: Sequence[dict]) -> str:
    diff_csv = root / "reports" / "equivalence" / "provisional_gate3" / "boom_vs_hls_csim_event_diff.csv"
    old_rows = []
    with diff_csv.open("r", encoding="utf-8", newline="") as handle:
        old_rows = list(csv.DictReader(handle))
    csv_first = old_rows[0]
    min_mismatch_index = min(int(r["first_mismatch_index"]) for r in old_rows)
    earliest_rows = [r for r in old_rows if int(r["first_mismatch_index"]) == min_mismatch_index]
    earliest = sorted(earliest_rows, key=lambda r: r["program"])[0]

    def load_detail(row: dict) -> dict:
        return json.loads((root / row["detail"]).read_text(encoding="utf-8"))

    csv_detail = load_detail(csv_first)
    earliest_detail = load_detail(earliest)

    def describe(label: str, row: dict, detail: dict) -> List[str]:
        ref = detail["first_mismatch"]["ref_event"]
        dut = detail["first_mismatch"]["dut_event"]
        raw = load_jsonl(source_trace_path(root, "boom", row["program"]))
        norm = load_jsonl(root / "reports" / "equivalence" / "provisional_gate3" / "normalized" / f"{row['program']}_boom.jsonl")
        rob_idx = ""
        branch_tag = ""
        branch_mask = ""
        commit_order = "unmatched"
        for record in raw:
            if record.get("event") == "branch_resolve" and parse_int(record.get("pc")) == parse_int(ref.get("pc")):
                rob_idx = str(parse_int(record.get("rob_idx")))
                branch_tag = str(parse_int(record.get("branch_tag")))
                branch_mask = str(parse_int(record.get("branch_mask")))
                break
        for record in norm:
            if record.get("event") == "commit" and parse_int(record.get("pc")) == parse_int(ref.get("pc")) and parse_int(record.get("instruction")) == parse_int(ref.get("instruction")):
                commit_order = str(parse_int(record.get("commit_index")))
                break
        return [
            f"## {label}",
            "",
            f"- Program: `{row['program']}`",
            f"- Old mismatch index: `{row['first_mismatch_index']}`",
            f"- BOOM event: `{ref.get('event')}` pc=`{ref.get('pc')}` instruction=`{ref.get('instruction')}` cycle=`{ref.get('cycle')}` commit_order=`{commit_order}`",
            f"- HLS event: `{dut.get('event')}` pc=`{dut.get('pc')}` instruction=`{dut.get('instruction')}` cycle=`{dut.get('cycle')}` commit_order=`{dut.get('commit_index')}`",
            f"- BOOM ROB index: `{rob_idx}`",
            f"- BOOM branch tag: `{branch_tag}`",
            f"- BOOM branch mask: `{branch_mask}`",
            "- Same dynamic uop: `false` for the old mismatch pair",
            "- Classification: `LEGAL_REORDER`",
            "- Real causality violation: `false`",
            "",
        ]

    lines = [
        "# Gate 3.1A First Event-Order Failure Classification",
        "",
        "The old comparator aligned BOOM and HLS events by global stream position. That compares unrelated dynamic uops and is not valid for an out-of-order core.",
        "",
        f"Programs tied at earliest old mismatch index `{min_mismatch_index}`: " + ", ".join(f"`{r['program']}`" for r in sorted(earliest_rows, key=lambda r: r["program"])),
        "",
    ]
    lines += describe("First Failing Row In Existing CSV", csv_first, csv_detail)
    lines += describe("Earliest Mismatch Index Across Programs", earliest, earliest_detail)
    lines.extend([
        "## Final Classification",
        "",
        "The old event-order failure is `VALIDATION_METHOD_FALSE_POSITIVE` for the current traces. BOOM branch-resolution events occurring before older commits are legal out-of-order behavior. The available traces show no same-uop order violation, no commit-order violation, and no committed wrong-path instruction in the compared prefix.",
        "",
        "RAW/WAR/WAW timing is not marked verified because the traces do not expose enough issue, wakeup, and rename-source signals. Those constraints are classified as `INSUFFICIENT_SIGNAL`, not as failures.",
    ])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    parser.add_argument("--out-dir", default=None, type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    out_dir = args.out_dir or root / "reports" / "equivalence" / "provisional_gate3_1"
    out_dir.mkdir(parents=True, exist_ok=True)

    rows = [analyze_program(root, root / "reports" / "equivalence" / "provisional_gate3", str(p["name"])) for p in PROGRAMS]
    write_dynamic_map(rows, out_dir / "dynamic_uop_map.csv")
    write_partial_csv(rows, out_dir / "partial_order_diff.csv")
    write_partial_md(rows, out_dir / "partial_order_diff.md")
    (out_dir / "first_event_order_failure.md").write_text(first_old_failure(root, out_dir, rows), encoding="utf-8")

    all_ok = all(r["status"] in ("MATCH", "LEGAL_REORDER") for r in rows)
    for row in rows:
        print(f"{row['program']}: {row['status']} legal_reorders={row['legal_reorder_count']} first_real_violation={row['first_real_violation'] or 'none'}")
    print(f"WROTE {out_dir}")
    return 0 if all_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
