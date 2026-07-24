#!/usr/bin/env python3
"""Shared helpers for Provisional Gate 3 trace normalization and comparison."""

from __future__ import annotations

import csv
import json
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


ENTRY_PC = 0x80000000
TOHOST_ADDR = 0x80000080

PROGRAMS = [
    {
        "name": "independent_alu",
        "boom_trace": "independent_alu_commit.jsonl",
        "hex": "independent_alu.hex",
        "prefix_commits": 8,
        "unsupported_pc": 0x80000020,
        "unsupported_instruction": 0x0062B023,
    },
    {
        "name": "raw_chain",
        "boom_trace": "raw_chain_cycle.jsonl",
        "hex": "raw_chain.hex",
        "prefix_commits": 8,
        "unsupported_pc": 0x80000020,
        "unsupported_instruction": 0x0062B023,
    },
    {
        "name": "branch_taken",
        "boom_trace": "branch_taken_cycle.jsonl",
        "hex": "branch_taken.hex",
        "prefix_commits": 9,
        "unsupported_pc": 0x80000028,
        "unsupported_instruction": 0x0062B023,
    },
    {
        "name": "branch_not_taken",
        "boom_trace": "branch_not_taken_cycle.jsonl",
        "hex": "branch_not_taken.hex",
        "prefix_commits": 10,
        "unsupported_pc": 0x80000028,
        "unsupported_instruction": 0x0062B023,
    },
    {
        "name": "nested_branch",
        "boom_trace": "nested_branch_cycle.jsonl",
        "hex": "nested_branch.hex",
        "prefix_commits": 10,
        "unsupported_pc": 0x80000030,
        "unsupported_instruction": 0x0062B023,
    },
]

PROGRAM_BY_NAME = {p["name"]: p for p in PROGRAMS}


def parse_int(value) -> Optional[int]:
    if value is None:
        return None
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        return value
    text = str(value).strip()
    if not text or text.lower() == "null":
        return None
    return int(text, 0)


def fmt_hex(value: Optional[int], width: int = 16) -> Optional[str]:
    if value is None:
        return None
    return f"0x{value:0{width}x}"


def fmt_pc(value: Optional[int]) -> Optional[str]:
    return fmt_hex(value, 16)


def fmt_inst(value: Optional[int]) -> Optional[str]:
    return fmt_hex(value, 8)


def load_jsonl(path: Path) -> List[dict]:
    records = []
    with path.open("r", encoding="utf-8") as handle:
        for line_no, line in enumerate(handle, 1):
            line = line.strip()
            if not line:
                continue
            try:
                records.append(json.loads(line))
            except json.JSONDecodeError as exc:
                raise ValueError(f"{path}:{line_no}: invalid JSONL: {exc}") from exc
    return records


def write_jsonl(path: Path, records: Iterable[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        for record in records:
            handle.write(json.dumps(record, sort_keys=True, separators=(",", ":")))
            handle.write("\n")


def trace_sequence(record: dict) -> int:
    seq = parse_int(record.get("trace_sequence_id"))
    if seq is not None:
        return seq
    return parse_int(record.get("event_index")) or 0


def cycle(record: dict) -> int:
    return parse_int(record.get("cycle")) or parse_int(record.get("original_cycle")) or 0


def record_order_key(record: dict) -> Tuple[int, int]:
    return cycle(record), trace_sequence(record)


def decode_instruction(inst: Optional[int]) -> Dict[str, object]:
    if inst is None:
        return {
            "mnemonic": "UNAVAILABLE",
            "category": "UNAVAILABLE",
            "hls_decode": False,
            "hls_execute": False,
            "hls_lsu_required": False,
            "hls_comparable": False,
            "unsupported_reason": "instruction unavailable in BOOM boot/ROM trace",
        }

    opcode = inst & 0x7F
    funct3 = (inst >> 12) & 0x7
    funct7 = (inst >> 25) & 0x7F

    def supported(mnemonic: str, category: str, execute: bool = True) -> Dict[str, object]:
        return {
            "mnemonic": mnemonic,
            "category": category,
            "hls_decode": True,
            "hls_execute": execute,
            "hls_lsu_required": False,
            "hls_comparable": execute,
            "unsupported_reason": "",
        }

    if opcode == 0x13:
        names = {0: "ADDI", 1: "SLLI", 2: "SLTI", 3: "SLTIU", 4: "XORI", 5: "SRLI/SRAI", 6: "ORI", 7: "ANDI"}
        return supported(names.get(funct3, "OP-IMM-ILLEGAL"), "ALU_IMM", funct3 in names)
    if opcode == 0x1B:
        names = {0: "ADDIW", 1: "SLLIW", 5: "SRLIW/SRAIW"}
        return supported(names.get(funct3, "OP-IMM-32-ILLEGAL"), "ALU_IMM_32", funct3 in names)
    if opcode == 0x33:
        names = {
            (0, 0x00): "ADD",
            (0, 0x20): "SUB",
            (1, 0x00): "SLL",
            (2, 0x00): "SLT",
            (3, 0x00): "SLTU",
            (4, 0x00): "XOR",
            (5, 0x00): "SRL",
            (5, 0x20): "SRA",
            (6, 0x00): "OR",
            (7, 0x00): "AND",
            (0, 0x01): "MUL",
        }
        return supported(names.get((funct3, funct7), "OP-ILLEGAL"), "ALU_REG", (funct3, funct7) in names)
    if opcode == 0x3B:
        names = {(0, 0x00): "ADDW", (0, 0x20): "SUBW", (1, 0x00): "SLLW", (5, 0x00): "SRLW", (5, 0x20): "SRAW"}
        return supported(names.get((funct3, funct7), "OP-32-ILLEGAL"), "ALU_REG_32", (funct3, funct7) in names)
    if opcode == 0x37:
        return supported("LUI", "ALU_IMM")
    if opcode == 0x17:
        return supported("AUIPC", "ALU_IMM")
    if opcode == 0x6F:
        return supported("JAL", "CONTROL")
    if opcode == 0x67:
        return supported("JALR", "CONTROL", funct3 == 0)
    if opcode == 0x63:
        names = {0: "BEQ", 1: "BNE", 4: "BLT", 5: "BGE", 6: "BLTU", 7: "BGEU"}
        return supported(names.get(funct3, "BRANCH-ILLEGAL"), "CONTROL", funct3 in names)
    if opcode == 0x03:
        names = {3: "LD"}
        return {
            "mnemonic": names.get(funct3, "LOAD-UNSUPPORTED"),
            "category": "LOAD",
            "hls_decode": funct3 in names,
            "hls_execute": False,
            "hls_lsu_required": True,
            "hls_comparable": False,
            "unsupported_reason": "HLS lsu_module is a no-op; loads are outside Provisional Gate 3 prefix scope",
        }
    if opcode == 0x23:
        names = {3: "SD"}
        return {
            "mnemonic": names.get(funct3, "STORE-UNSUPPORTED"),
            "category": "STORE",
            "hls_decode": funct3 in names,
            "hls_execute": False,
            "hls_lsu_required": True,
            "hls_comparable": False,
            "unsupported_reason": "HLS lsu_module is a no-op; retired store-to-tohost is the prefix stop boundary",
        }
    if opcode == 0x73:
        if funct3 == 0 and ((inst >> 20) & 0xFFF) == 0:
            return supported("ECALL", "SYSTEM")
        return supported("CSR", "SYSTEM")
    if opcode == 0x0F:
        return {
            "mnemonic": "FENCE",
            "category": "FENCE",
            "hls_decode": True,
            "hls_execute": False,
            "hls_lsu_required": True,
            "hls_comparable": False,
            "unsupported_reason": "memory-ordering behavior is outside the implemented HLS subset",
        }
    return {
        "mnemonic": "UNKNOWN",
        "category": "UNKNOWN",
        "hls_decode": False,
        "hls_execute": False,
        "hls_lsu_required": False,
        "hls_comparable": False,
        "unsupported_reason": f"opcode 0x{opcode:02x} is not in the implemented HLS decode subset",
    }


def is_unsupported(inst: Optional[int]) -> bool:
    return not bool(decode_instruction(inst)["hls_comparable"])


def source_trace_path(root: Path, source: str, program: str) -> Path:
    info = PROGRAM_BY_NAME[program]
    if source == "boom":
        return root / "reference" / "boom_traces" / str(info["boom_trace"])
    if source in ("hls_cpp", "hls_csim"):
        return root / "reference" / "hls_traces" / f"{program}_{source}.jsonl"
    raise ValueError(f"unknown source {source!r}")


def _base_metadata(source: str, program: str, base_cycle: Optional[int], records_in: int) -> dict:
    return {
        "event": "metadata",
        "phase": "start",
        "trace_schema_version": 1,
        "source": source,
        "program": program,
        "scope": "loaded_program_prefix_before_unsupported_tohost_store",
        "entry_pc": fmt_pc(ENTRY_PC),
        "tohost_addr": fmt_pc(TOHOST_ADDR),
        "cycle_base": base_cycle,
        "input_records": records_in,
        "unavailable_fields_are_null": True,
    }


def _commit_record(source: str, program: str, record: dict, commit_index: int, base_cycle: int) -> dict:
    pc = parse_int(record.get("pc"))
    inst = parse_int(record.get("instruction"))
    decoded = decode_instruction(inst)
    rec_cycle = cycle(record)
    rd_valid = bool(record.get("rd_valid"))
    return {
        "event": "commit",
        "trace_schema_version": 1,
        "source": source,
        "program": program,
        "commit_index": commit_index,
        "cycle": rec_cycle,
        "normalized_cycle": rec_cycle - base_cycle,
        "slot": parse_int(record.get("slot")),
        "pc": fmt_pc(pc),
        "instruction": fmt_inst(inst),
        "mnemonic": decoded["mnemonic"],
        "rd_valid": rd_valid,
        "rd": parse_int(record.get("rd")) if rd_valid else None,
        "rd_value": fmt_pc(parse_int(record.get("rd_value"))) if rd_valid else None,
        "exception": bool(record.get("exception")),
        "exception_cause": parse_int(record.get("exception_cause")),
        "branch_mispredict": record.get("branch_mispredict"),
        "arch_valid": record.get("arch_valid", True),
        "trace_sequence_id": trace_sequence(record),
    }


def _branch_record(source: str, program: str, record: dict, branch_index: int, base_cycle: int, pc_to_inst: Dict[int, int]) -> dict:
    pc = parse_int(record.get("pc"))
    inst = parse_int(record.get("instruction"))
    if inst is None and pc is not None:
        inst = pc_to_inst.get(pc)
    decoded = decode_instruction(inst)
    rec_cycle = cycle(record)
    target = parse_int(record.get("target"))
    if target is None:
        target = parse_int(record.get("redirect_pc"))
    return {
        "event": "branch",
        "trace_schema_version": 1,
        "source": source,
        "program": program,
        "branch_index": branch_index,
        "cycle": rec_cycle,
        "normalized_cycle": rec_cycle - base_cycle,
        "slot": parse_int(record.get("slot")),
        "pc": fmt_pc(pc),
        "instruction": fmt_inst(inst),
        "mnemonic": decoded["mnemonic"],
        "taken": record.get("taken"),
        "branch_mispredict": record.get("branch_mispredict"),
        "target": fmt_pc(target),
        "trace_sequence_id": trace_sequence(record),
    }


def normalize_boom(records: Sequence[dict], program: str) -> List[dict]:
    loaded_commits = []
    pc_to_inst: Dict[int, int] = {}
    for record in records:
        if record.get("event") != "commit":
            continue
        pc = parse_int(record.get("pc"))
        inst = parse_int(record.get("instruction"))
        if pc is None or pc < ENTRY_PC or inst is None:
            continue
        loaded_commits.append(record)
        pc_to_inst[pc] = inst

    if not loaded_commits:
        raise ValueError(f"{program}: no loaded-program commit records found")

    unsupported = next((record for record in loaded_commits if is_unsupported(parse_int(record.get("instruction")))), None)
    boundary_key = record_order_key(unsupported) if unsupported else (10**18, 10**18)
    base_cycle = cycle(loaded_commits[0])
    normalized: List[dict] = [_base_metadata("boom", program, base_cycle, len(records))]

    commit_index = 0
    branch_index = 0
    for record in sorted(records, key=record_order_key):
        event = record.get("event")
        pc = parse_int(record.get("pc"))
        if pc is None or pc < ENTRY_PC:
            continue
        if record_order_key(record) >= boundary_key:
            continue
        if event == "commit":
            inst = parse_int(record.get("instruction"))
            if inst is None or is_unsupported(inst):
                continue
            normalized.append(_commit_record("boom", program, record, commit_index, base_cycle))
            commit_index += 1
        elif event == "branch_resolve":
            normalized.append(_branch_record("boom", program, record, branch_index, base_cycle, pc_to_inst))
            branch_index += 1

    if unsupported is not None:
        unsupported_pc = parse_int(unsupported.get("pc"))
        unsupported_inst = parse_int(unsupported.get("instruction"))
        decoded = decode_instruction(unsupported_inst)
        normalized.append({
            "event": "unsupported_boundary",
            "trace_schema_version": 1,
            "source": "boom",
            "program": program,
            "cycle": cycle(unsupported),
            "normalized_cycle": cycle(unsupported) - base_cycle,
            "pc": fmt_pc(unsupported_pc),
            "instruction": fmt_inst(unsupported_inst),
            "mnemonic": decoded["mnemonic"],
            "reason": decoded["unsupported_reason"],
            "trace_sequence_id": trace_sequence(unsupported),
        })

    normalized.append({
        "event": "metadata",
        "phase": "end",
        "trace_schema_version": 1,
        "source": "boom",
        "program": program,
        "scope": "loaded_program_prefix_before_unsupported_tohost_store",
        "prefix_commit_records": commit_index,
        "branch_records": branch_index,
        "unsupported_boundary_pc": fmt_pc(parse_int(unsupported.get("pc"))) if unsupported else None,
        "unsupported_boundary_instruction": fmt_inst(parse_int(unsupported.get("instruction"))) if unsupported else None,
        "stop_reason": "unsupported_store_to_tohost_excluded" if unsupported else "no_unsupported_boundary_found",
    })
    return normalized


def normalize_hls(records: Sequence[dict], program: str, source: str) -> List[dict]:
    commits = [record for record in records if record.get("event") == "commit" and (parse_int(record.get("pc")) or 0) >= ENTRY_PC]
    if not commits:
        raise ValueError(f"{program}: no HLS loaded-program commit records found in {source}")

    base_cycle = cycle(commits[0])
    pc_to_inst = {parse_int(record.get("pc")): parse_int(record.get("instruction")) for record in commits}
    normalized: List[dict] = [_base_metadata(source, program, base_cycle, len(records))]
    commit_index = 0
    branch_index = 0

    for record in sorted(records, key=record_order_key):
        event = record.get("event")
        pc = parse_int(record.get("pc"))
        if pc is None or pc < ENTRY_PC:
            continue
        if event == "commit":
            inst = parse_int(record.get("instruction"))
            if inst is None or is_unsupported(inst):
                continue
            normalized.append(_commit_record(source, program, record, commit_index, base_cycle))
            commit_index += 1
        elif event in ("branch", "branch_resolve"):
            normalized.append(_branch_record(source, program, record, branch_index, base_cycle, pc_to_inst))
            branch_index += 1
        elif event == "unsupported_boundary":
            rec_cycle = cycle(record)
            inst = parse_int(record.get("instruction"))
            decoded = decode_instruction(inst)
            normalized.append({
                "event": "unsupported_boundary",
                "trace_schema_version": 1,
                "source": source,
                "program": program,
                "cycle": rec_cycle,
                "normalized_cycle": rec_cycle - base_cycle,
                "pc": fmt_pc(parse_int(record.get("pc"))),
                "instruction": fmt_inst(inst),
                "mnemonic": decoded["mnemonic"],
                "reason": record.get("reason") or decoded["unsupported_reason"],
                "trace_sequence_id": trace_sequence(record),
            })

    boundary = next((record for record in normalized if record.get("event") == "unsupported_boundary"), None)
    normalized.append({
        "event": "metadata",
        "phase": "end",
        "trace_schema_version": 1,
        "source": source,
        "program": program,
        "scope": "loaded_program_prefix_before_unsupported_tohost_store",
        "prefix_commit_records": commit_index,
        "branch_records": branch_index,
        "unsupported_boundary_pc": boundary.get("pc") if boundary else None,
        "unsupported_boundary_instruction": boundary.get("instruction") if boundary else None,
        "stop_reason": "prefix_limit_reached_before_unsupported_store" if boundary else "no_unsupported_boundary_recorded",
    })
    return normalized


def normalized_events(records: Sequence[dict], kind: str) -> List[dict]:
    events = [record for record in records if record.get("event") in ("commit", "branch")]
    if kind == "arch":
        events = [record for record in events if record.get("event") == "commit"]
    return events


def event_signature(record: dict, kind: str) -> Tuple[object, ...]:
    event = record.get("event")
    if event == "commit":
        sig: Tuple[object, ...] = (
            "commit",
            record.get("pc"),
            record.get("instruction"),
            bool(record.get("rd_valid")),
            record.get("rd") if record.get("rd_valid") else None,
            record.get("rd_value") if record.get("rd_valid") else None,
            bool(record.get("exception")),
            record.get("exception_cause") if record.get("exception") else None,
        )
    elif event == "branch":
        sig = (
            "branch",
            record.get("pc"),
            record.get("instruction"),
            record.get("taken"),
            record.get("branch_mispredict"),
        )
    else:
        sig = (event,)
    if kind == "cycle":
        sig = sig + (record.get("normalized_cycle"),)
    return sig


def compare_normalized(ref_records: Sequence[dict], dut_records: Sequence[dict], kind: str) -> Dict[str, object]:
    ref_events = normalized_events(ref_records, kind)
    dut_events = normalized_events(dut_records, kind)
    result: Dict[str, object] = {
        "kind": kind,
        "status": "PASS",
        "ref_events": len(ref_events),
        "dut_events": len(dut_events),
        "compared_events": min(len(ref_events), len(dut_events)),
        "first_mismatch_index": None,
        "first_mismatch": None,
    }
    if len(ref_events) != len(dut_events):
        result["status"] = "FAIL"
        result["first_mismatch_index"] = min(len(ref_events), len(dut_events))
        result["first_mismatch"] = {
            "reason": "event_count_mismatch",
            "ref_count": len(ref_events),
            "dut_count": len(dut_events),
        }
        return result
    for idx, (ref, dut) in enumerate(zip(ref_events, dut_events)):
        ref_sig = event_signature(ref, kind)
        dut_sig = event_signature(dut, kind)
        if ref_sig != dut_sig:
            result["status"] = "FAIL"
            result["first_mismatch_index"] = idx
            result["first_mismatch"] = {
                "reason": "signature_mismatch",
                "ref_signature": ref_sig,
                "dut_signature": dut_sig,
                "ref_event": ref,
                "dut_event": dut,
            }
            return result
    return result


def write_subset_csv(root: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as handle:
        fieldnames = [
            "program",
            "dynamic_index",
            "pc",
            "instruction",
            "mnemonic",
            "category",
            "rd_valid",
            "rd",
            "rd_value",
            "hls_decode",
            "hls_execute",
            "hls_lsu_required",
            "compared_in_prefix",
            "reason",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for program_info in PROGRAMS:
            program = str(program_info["name"])
            trace_path = source_trace_path(root, "boom", program)
            records = load_jsonl(trace_path)
            dynamic_index = 0
            boundary_seen = False
            for record in sorted(records, key=record_order_key):
                if record.get("event") != "commit":
                    continue
                pc = parse_int(record.get("pc"))
                inst = parse_int(record.get("instruction"))
                if pc is None or pc < ENTRY_PC or inst is None:
                    continue
                decoded = decode_instruction(inst)
                compared = (not boundary_seen) and bool(decoded["hls_comparable"])
                reason = "included in BOOM/HLS comparable loaded-program prefix" if compared else str(decoded["unsupported_reason"])
                writer.writerow({
                    "program": program,
                    "dynamic_index": dynamic_index,
                    "pc": fmt_pc(pc),
                    "instruction": fmt_inst(inst),
                    "mnemonic": decoded["mnemonic"],
                    "category": decoded["category"],
                    "rd_valid": str(bool(record.get("rd_valid"))).upper(),
                    "rd": parse_int(record.get("rd")) if record.get("rd_valid") else "",
                    "rd_value": fmt_pc(parse_int(record.get("rd_value"))) if record.get("rd_valid") else "",
                    "hls_decode": str(bool(decoded["hls_decode"])).upper(),
                    "hls_execute": str(bool(decoded["hls_execute"])).upper(),
                    "hls_lsu_required": str(bool(decoded["hls_lsu_required"])).upper(),
                    "compared_in_prefix": str(compared).upper(),
                    "reason": reason,
                })
                dynamic_index += 1
                if not decoded["hls_comparable"]:
                    boundary_seen = True
