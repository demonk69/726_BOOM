#!/usr/bin/env python3
"""Generate Gate 3.4 resource attribution and baseline inventories."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


REQUESTED_MODULES = [
    "synth_branch_tag_top",
    "synth_branch_mask_top",
    "synth_map_snapshot_top",
    "synth_free_list_rollback_top",
    "synth_busy_recovery_top",
    "synth_branch_kill_top",
    "synth_rename_top",
    "synth_rob_top",
    "synth_issue_top",
    "synth_lsu_top",
    "synth_core_step_top",
    "boom_core_top",
]


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def parse_time(path: Path) -> tuple[str, str]:
    runtime = ""
    peak = ""
    for line in read_text(path).splitlines():
        if line.startswith("runtime_seconds="):
            runtime = line.split("=", 1)[1]
        elif line.startswith("peak_memory_kb="):
            peak = line.split("=", 1)[1]
    return runtime, peak


def parse_runtime_from_md(path: Path) -> tuple[str, str]:
    runtime = ""
    peak = ""
    for line in read_text(path).splitlines():
        if "Runtime seconds" in line:
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            if len(cells) >= 2:
                runtime = cells[1]
        elif "Peak memory KB" in line:
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            if len(cells) >= 2:
                peak = cells[1]
    return runtime, peak


def parse_log(path: Path) -> tuple[str, int, int]:
    last_pass = ""
    warnings = 0
    partition_count = 0
    for line in read_text(path).splitlines():
        match = re.search(r"Finished ([^:]+):", line)
        if match:
            last_pass = match.group(1)
        if "WARNING:" in line:
            warnings += 1
        if "Partitioning array '" in line:
            partition_count += 1
    return last_pass, warnings, partition_count


def parse_report(path: Path) -> dict[str, str]:
    result = {
        "estimated_period_ns": "",
        "lut": "",
        "ff": "",
        "bram_18k": "",
        "dsp": "",
        "latency": "",
        "interval": "",
        "pipeline": "",
    }
    lines = read_text(path).splitlines()
    for line in lines:
        if "|ap_clk" in line:
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            if len(cells) >= 3:
                result["estimated_period_ns"] = cells[2].replace(" ns", "")
        if line.strip().startswith("|Total"):
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            if len(cells) >= 6 and cells[1].replace("-", "").isdigit():
                result["bram_18k"] = cells[1]
                result["dsp"] = cells[2]
                result["ff"] = cells[3]
                result["lut"] = cells[4]
                break
    for idx, line in enumerate(lines):
        if "|  Latency (cycles) |" in line:
            for candidate in lines[idx + 1:idx + 10]:
                if not candidate.strip().startswith("|") or candidate.strip().startswith("|   min"):
                    continue
                cells = [cell.strip() for cell in candidate.strip().strip("|").split("|")]
                if len(cells) >= 7 and cells[0] not in ("", "Latency (cycles)"):
                    result["latency"] = f"{cells[0]}..{cells[1]}"
                    result["interval"] = f"{cells[4]}..{cells[5]}"
                    result["pipeline"] = cells[6]
                    return result
    return result


def parse_summary_csv(path: Path) -> dict[str, dict[str, str]]:
    if not path.exists():
        return {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        return {row.get("module", row.get("function", "")): row for row in csv.DictReader(handle)}


def parse_resource_delta(path: Path) -> dict[tuple[str, str], dict[str, str]]:
    if not path.exists():
        return {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        return {(row["scope"], row["metric"]): row for row in csv.DictReader(handle)}


def top_report(root: Path, gate: str) -> Path:
    return root / f"boom_hls_{gate}_baseline" / "solution_baseline" / "syn" / "report" / "boom_core_top_csynth.rpt"


def write_baseline_resources(root: Path, out_dir: Path) -> None:
    rows = []
    configs = [
        ("gate3_2", "boom_core_top", top_report(root, "gate3_2"), root / "reports/gate3_3/baseline_csynth_gate3_2.md"),
        ("gate3_3", "boom_core_top", top_report(root, "gate3_3"), root / "reports/gate3_3/baseline_csynth_summary.md"),
        ("gate3_3", "boom_core_step_top", root / "boom_hls_gate3_3_boom_core_step_top/solution_top/syn/report/boom_core_step_top_csynth.rpt", root / "reports/gate3_3/step_top_csynth_summary.md"),
    ]
    for gate, top, report, md in configs:
        metrics = parse_report(report)
        runtime, peak = parse_runtime_from_md(md)
        rows.append({
            "gate": gate,
            "top": top,
            "status": "PASS" if report.exists() else "MISSING_REPORT",
            "runtime_seconds": runtime,
            "peak_memory_kb": peak,
            "estimated_period_ns": metrics["estimated_period_ns"],
            "lut": metrics["lut"],
            "ff": metrics["ff"],
            "bram_18k": metrics["bram_18k"],
            "dsp": metrics["dsp"],
            "pipeline": metrics["pipeline"],
            "report_path": str(report.relative_to(root)) if report.exists() else str(report),
        })
    write_csv(out_dir / "baseline_resources.csv", rows)


def classify_ram(name: str) -> tuple[str, str]:
    if "br_snapshots" in name:
        return "rename_map_snapshots", "Gate 3.3 active snapshot RAM; check port suffix for access structure"
    if "br_alloc_lists" in name:
        return "branch_allocation_lists", "Gate 3.3 per-branch physical-register allocation bitmap"
    if "branch_br_mask" in name or "branch_mask" in name:
        return "branch_mask_storage", "Branch mask field stored inside in-flight uop or LSU structures"
    if "busy_table" in name:
        return "busy_table", "Integer physical register readiness bitmap"
    if "free_list" in name:
        return "free_list", "Integer free-list queue storage"
    if "map_table" in name:
        return "rename_map_table", "Integer rename map storage"
    if "issue_alu_iq" in name:
        return "issue_queue", "Integer issue queue field storage"
    if "ldq" in name or "stq" in name:
        return "lsu_queue", "Minimal LSU queue field storage"
    if "rob_entries" in name:
        return "rob", "ROB entry field storage"
    return "other", "Generated HLS field RAM"


def parse_ram_module(path: Path, root: Path, gate: str) -> dict[str, str]:
    text = read_text(path)
    def param(name: str) -> str:
        match = re.search(rf"parameter {name} = (\d+);", text)
        return match.group(1) if match else ""
    module_match = re.search(r"module\s+([^\s(]+)", text)
    module_name = module_match.group(1) if module_match else path.stem
    component, notes = classify_ram(path.name)
    port_match = re.search(r"RAM_AUTO_([^./]+)\.v$", path.name)
    return {
        "gate": gate,
        "component": component,
        "rtl_module": module_name,
        "data_width": param("DataWidth"),
        "address_width": param("AddressWidth"),
        "address_range": param("AddressRange"),
        "ports": port_match.group(1) if port_match else "unknown",
        "rtl_storage": "reg_array_ram" if "reg [DataWidth-1:0] ram" in text else "unknown",
        "complete_partition": "false",
        "bram_inferred_by_summary": "false",
        "path": str(path.relative_to(root)),
        "evidence_kind": "RTL_INSTANCE",
        "notes": notes,
    }


def write_rtl_inventory(root: Path, out_dir: Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for gate in ("gate3_2", "gate3_3"):
        base = root / f"boom_hls_{gate}_baseline" / "solution_baseline" / "syn" / "verilog"
        if not base.exists():
            continue
        for path in sorted(base.glob("*RAM_AUTO*.v")):
            rows.append(parse_ram_module(path, root, gate))
    write_csv(out_dir / "rtl_structure_inventory.csv", rows)
    return rows


def write_auto_partition_inventory(root: Path, out_dir: Path) -> None:
    rows = []
    for gate in ("gate3_2", "gate3_3"):
        project = root / f"boom_hls_{gate}_baseline" / "solution_baseline"
        for log_name in ("solution_baseline.log", ".autopilot/db/autopilot.flow.log"):
            log_path = project / log_name
            if not log_path.exists():
                continue
            text = read_text(log_path)
            rows.append({
                "gate": gate,
                "project": f"boom_hls_{gate}_baseline",
                "log_path": str(log_path.relative_to(root)),
                "auto_partition_mode": "default" if "Auto array partition mode is set into default" in text or "reflow-auto-array-partition-mode=default" in text else "unknown",
                "analysis_started": str("Starting automatic array partition analysis" in text).upper(),
                "partition_record_count": str(text.count("Partitioning array '")),
                "partition_records": "none" if "Partitioning array '" not in text else "see log",
                "notes": "No explicit automatic array partition records found" if "Partitioning array '" not in text else "Explicit partition records found",
            })
    write_csv(out_dir / "automatic_partition_inventory.csv", rows)


def add_attr(rows: list[dict[str, str]], **kwargs: str) -> None:
    fields = [
        "component", "source_file", "function", "state_or_array", "estimated_lut",
        "estimated_ff", "estimated_bram", "gate3_2_structure", "gate3_3_structure",
        "suspected_cause", "confidence", "optimization_candidate", "risk",
        "evidence_kind", "evidence_path",
    ]
    rows.append({field: kwargs.get(field, "") for field in fields})


def write_resource_attribution(root: Path, out_dir: Path, rtl_rows: list[dict[str, str]]) -> None:
    delta = parse_resource_delta(root / "reports/gate3_3/resource_delta.csv")
    module3 = parse_summary_csv(root / "reports/gate3_3/module_csynth_summary.csv")
    branch_diag = parse_summary_csv(root / "reports/gate3_3/branch_recovery_csynth_diagnostics.csv")
    rows: list[dict[str, str]] = []

    core_lut_delta = delta.get(("boom_core_top", "LUT"), {}).get("delta", "42661")
    core_ff_delta = delta.get(("boom_core_top", "FF"), {}).get("delta", "626")
    add_attr(
        rows,
        component="full_core_delta",
        source_file="src/boom_core_merged.cpp",
        function="boom_core_cycle_io",
        state_or_array="BoomCoreState branch recovery additions",
        estimated_lut=core_lut_delta,
        estimated_ff=core_ff_delta,
        estimated_bram="0",
        gate3_2_structure="boom_core_cycle_io instance LUT 40156; no active branch recovery control modules",
        gate3_3_structure="boom_core_cycle_io instance LUT 82817 with branch_module/recover/kill helpers",
        suspected_cause="all top-level LUT growth is reported under the synthesized instance hierarchy, not FIFO/DSP/BRAM summaries",
        confidence="HIGH",
        optimization_candidate="target recovery helper boundaries and branch kill/rollback structures",
        risk="medium: changes touch recovery cycle semantics",
        evidence_kind="REPORT_DIRECT",
        evidence_path="reports/gate3_3/resource_delta.csv; boom_core_top_csynth.rpt",
    )
    for name, component, risk in [
        ("branch_module", "branch_recovery_dispatch", "medium"),
        ("recover_mispredict", "mispredict_recovery_path", "high"),
        ("kill_issue_state", "issue_branch_kill", "medium"),
        ("kill_lsu_state", "lsu_branch_kill", "medium"),
        ("kill_rob_younger_than", "rob_branch_kill", "medium"),
        ("clear_resolved_masks_in_state", "resolved_mask_clear", "low"),
        ("compact_issue_queue", "issue_queue_compaction", "medium"),
    ]:
        row = branch_diag.get(name, {})
        add_attr(
            rows,
            component=component,
            source_file="src/branch.cpp" if name != "compact_issue_queue" else "src/branch.cpp/src/issue.cpp",
            function=name,
            state_or_array=row.get("notes", "branch recovery helper"),
            estimated_lut=row.get("LUT", "not_isolated"),
            estimated_ff=row.get("FF", "not_isolated"),
            estimated_bram=row.get("BRAM_18K", "0"),
            gate3_2_structure="absent or coarse full-flush behavior",
            gate3_3_structure="separate synthesized helper visible in branch recovery diagnostics",
            suspected_cause="new branch recovery control/data update cone; report hierarchy overlaps and is not additive",
            confidence="HIGH" if row else "MEDIUM",
            optimization_candidate="function-boundary audit; local kill bitmap; packed allocation bitmap" if name in ("recover_mispredict", "kill_issue_state", "compact_issue_queue") else "structure audit",
            risk=risk,
            evidence_kind="REPORT_DIRECT",
            evidence_path=row.get("report_path", "reports/gate3_3/branch_recovery_csynth_diagnostics.csv"),
        )
    rename = module3.get("synth_rename_top", {})
    rename_delta = delta.get(("synth_rename_top", "LUT"), {}).get("delta", "191")
    add_attr(
        rows,
        component="rename_branch_tag_and_snapshot_write",
        source_file="src/rename.cpp",
        function="rename_module",
        state_or_array="active_mask/tag_valid/snapshot_valid/br_snapshots",
        estimated_lut=rename_delta,
        estimated_ff=delta.get(("synth_rename_top", "FF"), {}).get("delta", "214"),
        estimated_bram="0",
        gate3_2_structure="no active tag allocation/snapshot write path",
        gate3_3_structure=f"synth_rename_top LUT {rename.get('LUT', '')}; snapshot RAM is 256x8 RAM_AUTO_1R1W",
        suspected_cause="low free-tag scan, snapshot write loop, allocation-list update for active tags",
        confidence="HIGH",
        optimization_candidate="snapshot storage and packed allocation-list experiments, but rename delta is not the dominant top-level source",
        risk="medium",
        evidence_kind="REPORT_DIRECT+RTL_INSTANCE",
        evidence_path="reports/gate3_3/module_csynth_summary.csv; rtl_structure_inventory.csv",
    )
    issue_delta = delta.get(("synth_issue_top", "LUT"), {}).get("delta", "3969")
    add_attr(
        rows,
        component="issue_branch_mask_kill_refresh",
        source_file="src/issue.cpp/src/branch.cpp",
        function="issue_module; kill_issue_state; compact_issue_queue",
        state_or_array="issue.alu_iq.entries[*].uop.branch.br_mask",
        estimated_lut=issue_delta,
        estimated_ff=delta.get(("synth_issue_top", "FF"), {}).get("delta", "22"),
        estimated_bram="0",
        gate3_2_structure="IQ select/compact without branch kill/clear",
        gate3_3_structure="IQ branch mask RAM_AUTO_1R1W plus kill and compaction helpers",
        suspected_cause="entry-wise branch-mask compare plus compaction/state rewrite",
        confidence="HIGH",
        optimization_candidate="D1/D4 local kill bitmap and shared mask computation",
        risk="medium: wrong-path uop must not survive with side effects",
        evidence_kind="REPORT_DIRECT+RTL_INSTANCE",
        evidence_path="reports/gate3_3/resource_delta.csv; branch_recovery_csynth_diagnostics.csv",
    )
    add_attr(
        rows,
        component="snapshot_storage",
        source_file="include/boom_state.hpp/src/rename.cpp/src/branch.cpp",
        function="save_map_snapshot; restore_map_snapshot",
        state_or_array="rename.int_map_table.br_snapshots[32][8]",
        estimated_lut="not_isolated_storage_logic_in_rename_and_recover",
        estimated_ff="not_isolated",
        estimated_bram="0",
        gate3_2_structure="present as unused 0R0W RAM in generated RTL",
        gate3_3_structure="256x8 RAM_AUTO_1R1W reg-array RAM; no complete partition; BRAM total unchanged",
        suspected_cause="restore loop reads 32 snapshot entries through dynamic tag address; storage itself is not dominant",
        confidence="HIGH",
        optimization_candidate="A2/A4 only after access-port proof; packed row may trade mux shape for storage width",
        risk="medium: restore cycle semantics and x0 mapping must remain exact",
        evidence_kind="RTL_INSTANCE+REPORT_DIRECT",
        evidence_path="rtl_structure_inventory.csv; boom_core_top_*br_snapshots*_RAM_AUTO_1R1W.v",
    )
    add_attr(
        rows,
        component="allocation_list_rollback",
        source_file="include/boom_state.hpp/src/rename.cpp/src/branch.cpp",
        function="record_pdst_for_active_branches; rollback_free_list; prune_recovered_tags",
        state_or_array="branch_state.br_alloc_lists[8][52]",
        estimated_lut="included_in_recover_mispredict_11724_and_branch_module_15506",
        estimated_ff="included_in_branch_state_storage",
        estimated_bram="0",
        gate3_2_structure="absent",
        gate3_3_structure="416x1 RAM_AUTO_1R1W reg-array RAM plus nested rollback/clear loops",
        suspected_cause="rollback scans 51 physical regs, checks map membership, duplicate-free-list membership, then clears allocation bits across active tags",
        confidence="HIGH",
        optimization_candidate="B1 packed ap_uint<52> bitmap and B4 duplicate-safe set operation candidate",
        risk="high: duplicate-safe recycle and same-cycle rollback semantics must be preserved",
        evidence_kind="RTL_INSTANCE+LOG_EVIDENCE+ENGINEERING_INFERENCE",
        evidence_path="rtl_structure_inventory.csv; solution_baseline.log partial-write records; src/branch.cpp",
    )
    add_attr(
        rows,
        component="busy_recovery",
        source_file="src/branch.cpp",
        function="rebuild_busy_after_recovery",
        state_or_array="rename.int_free_list.busy_table[52] and rob.entries[32]",
        estimated_lut="included_in_recover_mispredict_11724",
        estimated_ff="not_isolated",
        estimated_bram="0",
        gate3_2_structure="no branch-recovery busy rebuild",
        gate3_3_structure="52-bit clear loop plus 32-entry ROB scan after rollback",
        suspected_cause="full busy-table clear and ROB scan are inlined into recover_mispredict; normal cycles still carry synthesized recovery control cone in the helper",
        confidence="MEDIUM",
        optimization_candidate="C1 recovery_enable boundary and C2 single recovery bitmap candidate",
        risk="medium/high: wakeup/issue timing evidence is insufficient",
        evidence_kind="LOG_EVIDENCE+ENGINEERING_INFERENCE",
        evidence_path="recover_mispredict_csynth.rpt loop table; solution_baseline.log inlining records",
    )
    write_csv(out_dir / "resource_attribution.csv", rows)
    write_resource_md(out_dir / "resource_attribution.md", rows, rtl_rows)


def write_resource_md(path: Path, rows: list[dict[str, str]], rtl_rows: list[dict[str, str]]) -> None:
    snapshots = [r for r in rtl_rows if r["component"] == "rename_map_snapshots" and r["gate"] == "gate3_3"]
    allocs = [r for r in rtl_rows if r["component"] == "branch_allocation_lists" and r["gate"] == "gate3_3"]
    lines = [
        "# Gate 3.4 Resource Attribution",
        "",
        "## Summary",
        "",
        "Gate 3.3 increases conservative `boom_core_top` LUTs by 42661 over Gate 3.2. The Vitis top report accounts for this entirely in the synthesized instance hierarchy: the top `Instance` LUT line moves from 40156 to 82817 while FIFO, expression, mux, BRAM, and DSP totals are effectively unchanged.",
        "",
        "The direct branch recovery diagnostics are not additive, but they identify the new dominant cones: `branch_module` reports 15506 LUT, `recover_mispredict` reports 11724 LUT, `kill_issue_state` reports 5799 LUT, `compact_issue_queue` reports 5473 LUT, `kill_lsu_state` reports 2269 LUT, `kill_rob_younger_than` reports 1501 LUT, and `clear_resolved_masks_in_state` reports 366 LUT.",
        "",
        "## Storage Findings",
        "",
    ]
    if snapshots:
        r = snapshots[0]
        lines.append(f"- Snapshot storage is `{r['address_range']}x{r['data_width']}` `{r['ports']}` `{r['rtl_storage']}` at `{r['path']}`. It is not complete-partitioned and did not add BRAM in the accepted top.")
    if allocs:
        r = allocs[0]
        lines.append(f"- `br_alloc_lists` storage is `{r['address_range']}x{r['data_width']}` `{r['ports']}` `{r['rtl_storage']}` at `{r['path']}`. It is not complete-partitioned and did not add BRAM in the accepted top.")
    lines += [
        "",
        "## Attribution Table",
        "",
        "| Component | Evidence | Estimated LUT | Confidence | Candidate | Risk |",
        "|---|---|---:|---|---|---|",
    ]
    for row in rows:
        lines.append(f"| {row['component']} | {row['evidence_kind']} | {row['estimated_lut']} | {row['confidence']} | {row['optimization_candidate']} | {row['risk']} |")
    lines += [
        "",
        "## Optimization Direction",
        "",
        "The first safe experiments should isolate structure before directives: packed allocation bitmaps, local branch-kill bitmaps, and explicit recovery-enable boundaries. Snapshot storage directives are lower priority because the accepted RTL already maps snapshots as RAM_AUTO_1R1W and the rename-module LUT delta is only 191.",
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_module_baseline_placeholder(root: Path, out_dir: Path) -> None:
    module3 = parse_summary_csv(root / "reports/gate3_3/module_csynth_summary.csv")
    report_map = {
        "synth_rename_top": module3.get("synth_rename_top", {}),
        "synth_rob_top": module3.get("synth_rob_top", {}),
        "synth_issue_top": module3.get("synth_issue_top", {}),
        "synth_lsu_top": module3.get("synth_lsu_top", {}),
        "synth_core_step_top": module3.get("synth_core_step_top", {}),
    }
    rows = []
    for module in REQUESTED_MODULES:
        gate4_report = root / f"boom_hls_gate3_4_{module}" / "solution_module" / "syn" / "report" / f"{module}_csynth.rpt"
        gate4_log = root / "reports" / "gate3_4" / "module_csynth" / f"{module}.log"
        gate4_time = root / "reports" / "gate3_4" / "module_csynth" / f"{module}.time"
        if gate4_report.exists():
            metrics = parse_report(gate4_report)
            runtime, peak = parse_time(gate4_time)
            _last_pass, warnings, partition_count = parse_log(gate4_log)
            rows.append({
                "module": module,
                "status": "PASS",
                "runtime_seconds": runtime,
                "peak_memory_kb": peak,
                "estimated_period_ns": metrics["estimated_period_ns"],
                "lut": metrics["lut"],
                "ff": metrics["ff"],
                "bram_18k": metrics["bram_18k"],
                "dsp": metrics["dsp"],
                "latency": metrics["latency"],
                "interval": metrics["interval"],
                "automatic_partition_count": str(partition_count),
                "warning_count": str(warnings),
                "report_path": str(gate4_report.relative_to(root)),
            })
            continue
        proxy = report_map.get(module, {})
        if module == "boom_core_top":
            metrics = parse_report(top_report(root, "gate3_3"))
            runtime, peak = parse_runtime_from_md(root / "reports/gate3_3/baseline_csynth_summary.md")
            rows.append({
                "module": module,
                "status": "GATE3_3_PROXY_PASS_GATE3_4_NOT_RUN",
                "runtime_seconds": runtime,
                "peak_memory_kb": peak,
                "estimated_period_ns": metrics["estimated_period_ns"],
                "lut": metrics["lut"],
                "ff": metrics["ff"],
                "bram_18k": metrics["bram_18k"],
                "dsp": metrics["dsp"],
                "latency": metrics["latency"],
                "interval": metrics["interval"],
                "automatic_partition_count": "0",
                "warning_count": "60",
                "report_path": "boom_hls_gate3_3_baseline/solution_baseline/syn/report/boom_core_top_csynth.rpt",
            })
        elif proxy:
            rows.append({
                "module": module,
                "status": "GATE3_3_PROXY_PASS_GATE3_4_NOT_RUN",
                "runtime_seconds": proxy.get("runtime", ""),
                "peak_memory_kb": proxy.get("peak_memory", ""),
                "estimated_period_ns": proxy.get("estimated_period", "").replace(" ns", ""),
                "lut": proxy.get("LUT", ""),
                "ff": proxy.get("FF", ""),
                "bram_18k": proxy.get("BRAM", ""),
                "dsp": proxy.get("DSP", ""),
                "latency": "?..?",
                "interval": "?..?",
                "automatic_partition_count": "0",
                "warning_count": proxy.get("warnings", ""),
                "report_path": proxy.get("report_path", ""),
            })
        else:
            rows.append({
                "module": module,
                "status": "NOT_RUN_FRAMEWORK_READY",
                "runtime_seconds": "",
                "peak_memory_kb": "",
                "estimated_period_ns": "",
                "lut": "",
                "ff": "",
                "bram_18k": "",
                "dsp": "",
                "latency": "",
                "interval": "",
                "automatic_partition_count": "",
                "warning_count": "",
                "report_path": f"boom_hls_gate3_4_{module}/solution_module/syn/report/{module}_csynth.rpt",
            })
    write_csv(out_dir / "module_baseline.csv", rows)


def write_experiment_placeholders(out_dir: Path) -> None:
    specs = [
        ("snapshot_storage", "A", "Snapshot Storage", ["A0 Gate 3.3 original", "A1 independent 2D array", "A2 BRAM mapping", "A3 logical-register banks", "A4 packed row"]),
        ("allocation_rollback", "B", "Allocation-list Rollback", ["B0 Gate 3.3 original", "B1 packed ap_uint bitmap", "B2 hierarchical bitmap", "B3 rollback event", "B4 duplicate-safe set logic"]),
        ("busy_recovery", "C", "Busy Table Recovery", ["C0 Gate 3.3 original", "C1 recovery_enable boundary", "C2 recovery mask single-point update", "C3 busy snapshot comparison"]),
        ("branch_kill_network", "D", "Branch Mask Kill Network", ["D0 Gate 3.3 original", "D1 shared kill mask", "D2 resolved/mispredict split", "D3 fixed reduction tree", "D4 local kill bitmap"]),
        ("structure_inline", "E", "Structure And Inline", ["E0 Gate 3.3 original", "E1 independent branch state", "E2 separate snapshot/alloc arrays", "E3 control/state split", "E4 inline off", "E5 inline small helpers"]),
    ]
    exp_dir = out_dir / "experiments"
    exp_dir.mkdir(parents=True, exist_ok=True)
    for filename, group, title, variants in specs:
        rows = []
        for variant in variants:
            rows.append({
                "experiment": variant.split()[0],
                "description": " ".join(variant.split()[1:]),
                "status": "BASELINE_RECORDED" if variant.endswith("original") else "NOT_RUN_RESOURCE_ATTRIBUTION_PHASE",
                "functional_status": "Gate 3.3 preserved" if variant.endswith("original") else "not run",
                "csynth_status": "Gate 3.3 baseline PASS" if variant.endswith("original") else "not run",
                "lut": "83286" if variant.endswith("original") else "",
                "ff": "16611" if variant.endswith("original") else "",
                "bram_18k": "16" if variant.endswith("original") else "",
                "dsp": "3" if variant.endswith("original") else "",
                "estimated_period_ns": "5.898" if variant.endswith("original") else "",
                "risk": "none" if variant.endswith("original") else "requires single-variable implementation and full Gate 3.3 validation",
            })
        write_csv(exp_dir / f"{filename}.csv", rows)
        lines = [
            f"# Gate 3.4 Experiment {group}: {title}",
            "",
            "Status: framework defined; only A0/B0/C0/D0/E0 Gate 3.3 baseline is recorded in the resource-attribution phase.",
            "",
            "No candidate from this group is accepted until it passes the full Gate 3.3 regression and trace matrix plus csynth.",
            "",
            "| Experiment | Status | Risk |",
            "|---|---|---|",
        ]
        for row in rows:
            lines.append(f"| {row['experiment']} | {row['status']} | {row['risk']} |")
        (exp_dir / f"{filename}.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def append_module_row(output: Path, module: str, status: str, report: Path, log: Path, time_log: Path) -> None:
    metrics = parse_report(report)
    runtime, peak = parse_time(time_log)
    _last_pass, warnings, partition_count = parse_log(log)
    row = {
        "module": module,
        "status": status,
        "runtime_seconds": runtime,
        "peak_memory_kb": peak,
        "estimated_period_ns": metrics["estimated_period_ns"],
        "lut": metrics["lut"],
        "ff": metrics["ff"],
        "bram_18k": metrics["bram_18k"],
        "dsp": metrics["dsp"],
        "latency": metrics["latency"],
        "interval": metrics["interval"],
        "automatic_partition_count": str(partition_count),
        "warning_count": str(warnings),
        "report_path": str(report),
    }
    exists = output.exists()
    with output.open("a", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(row.keys()))
        if not exists:
            writer.writeheader()
        writer.writerow(row)


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    parser.add_argument("--append-module-row", type=Path)
    parser.add_argument("--module", default="")
    parser.add_argument("--status", default="")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--log", type=Path)
    parser.add_argument("--time-log", type=Path)
    args = parser.parse_args()

    root = args.root.resolve()
    if args.append_module_row:
        if not (args.module and args.report and args.log and args.time_log):
            raise SystemExit("--append-module-row requires --module, --report, --log, and --time-log")
        append_module_row(args.append_module_row, args.module, args.status, args.report, args.log, args.time_log)
        return 0

    out_dir = root / "reports" / "gate3_4"
    out_dir.mkdir(parents=True, exist_ok=True)
    write_baseline_resources(root, out_dir)
    rtl_rows = write_rtl_inventory(root, out_dir)
    write_auto_partition_inventory(root, out_dir)
    write_resource_attribution(root, out_dir, rtl_rows)
    write_module_baseline_placeholder(root, out_dir)
    write_experiment_placeholders(out_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
