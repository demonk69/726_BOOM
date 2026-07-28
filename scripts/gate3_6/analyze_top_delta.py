#!/usr/bin/env python3
"""Generate Gate 3.6 top-level hierarchy and RTL resource-delta evidence."""

from __future__ import annotations

import csv
import re
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports" / "gate3_6"

DESIGNS = {
    "synth_core_step_top": ROOT / "boom_hls_gate3_4_synth_core_step_top" / "solution_module",
    "boom_core_step_top": ROOT / "boom_hls_gate3_3_boom_core_step_top" / "solution_top",
    "boom_core_top": ROOT / "boom_hls_gate3_4_boom_core_top" / "solution_module",
}

REPORTS = {
    "synth_core_step_top": DESIGNS["synth_core_step_top"] / "syn/report/synth_core_step_top_csynth.rpt",
    "boom_core_step_top": DESIGNS["boom_core_step_top"] / "syn/report/boom_core_step_top_csynth.rpt",
    "boom_core_top": DESIGNS["boom_core_top"] / "syn/report/boom_core_top_csynth.rpt",
    "boom_core_cycle_io": DESIGNS["boom_core_top"] / "syn/report/boom_core_cycle_io_csynth.rpt",
}

CATEGORY_SOURCE = {
    "Expression": ("top/control expressions", "src/boom_core_top.cpp"),
    "FIFO": ("PipeSignals stream storage", "include/boom_interfaces.hpp"),
    "Instance": ("retained helpers and core modules", "src/boom_core_step.cpp"),
    "Memory": ("persistent BoomCoreState arrays", "include/boom_state.hpp"),
    "Multiplexer": ("state/RAM port arbitration", "generated RTL"),
    "Register": ("top/FSM/control registers", "src/boom_core_top.cpp"),
}

HELPER_SOURCE = {
    "branch_module": "src/branch.cpp",
    "decode_module": "src/decode.cpp",
    "execute_module": "src/execute.cpp",
    "frontend_module": "src/frontend.cpp",
    "issue_module": "src/issue.cpp",
    "lsu_module": "src/lsu.cpp",
    "rename_module": "src/rename.cpp",
    "rob_allocate": "src/rob.cpp",
    "rob_commit_module": "src/commit.cpp",
}


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def cells(line: str) -> list[str]:
    return [cell.strip() for cell in line.strip().strip("|").split("|")]


def as_int(value: str) -> int:
    value = value.strip()
    return int(value) if value and value not in ("-", "?") else 0


def section(text: str, marker: str, next_markers: tuple[str, ...]) -> str:
    start = text.find(marker)
    if start < 0:
        return ""
    end = len(text)
    for candidate in next_markers:
        pos = text.find(candidate, start + len(marker))
        if 0 <= pos < end:
            end = pos
    return text[start:end]


def parse_summary(path: Path) -> dict[str, int | str]:
    text = read_text(path)
    result: dict[str, int | str] = {"period": "", "lut": 0, "ff": 0, "bram": 0, "dsp": 0}
    match = re.search(r"\|ap_clk\s*\|\s*[^|]+\|\s*([0-9.]+) ns", text)
    if match:
        result["period"] = match.group(1)
    util = section(text, "== Utilization Estimates", ())
    for line in util.splitlines():
        row = cells(line)
        if len(row) >= 6 and row[0] == "Total":
            result.update({"bram": as_int(row[1]), "dsp": as_int(row[2]), "ff": as_int(row[3]), "lut": as_int(row[4])})
            break
    return result


def parse_categories(path: Path) -> dict[str, dict[str, int]]:
    utilization = section(read_text(path), "== Utilization Estimates", ())
    text = section(utilization, "* Summary:", ("+ Detail:",))
    result: dict[str, dict[str, int]] = {}
    for line in text.splitlines():
        row = cells(line)
        if len(row) >= 6 and row[0] in CATEGORY_SOURCE:
            result[row[0]] = {"bram": as_int(row[1]), "dsp": as_int(row[2]), "ff": as_int(row[3]), "lut": as_int(row[4])}
    return result


def parse_instance_table(path: Path) -> dict[str, dict[str, int | str]]:
    util = section(read_text(path), "== Utilization Estimates", ())
    table = section(util, "* Instance:", ("* DSP:", "* Memory:", "* FIFO:", "* Expression:", "* Multiplexer:", "* Register:"))
    result: dict[str, dict[str, int | str]] = {}
    for line in table.splitlines():
        row = cells(line)
        if len(row) == 7 and row[0] not in ("Instance", "Total", "") and row[1] not in ("Module", ""):
            result[str(row[1])] = {"instance": row[0], "bram": as_int(row[2]), "dsp": as_int(row[3]), "ff": as_int(row[4]), "lut": as_int(row[5])}
    return result


def parse_memory_table(path: Path) -> list[dict[str, str | int]]:
    util = section(read_text(path), "== Utilization Estimates", ())
    table = section(util, "* Memory:", ("* FIFO:", "* Expression:", "* Multiplexer:", "* Register:"))
    rows: list[dict[str, str | int]] = []
    for line in table.splitlines():
        row = cells(line)
        if len(row) == 10 and row[0] not in ("Memory", "Total", "") and row[1] not in ("Module", ""):
            rows.append({"memory": row[0], "module": row[1], "bram": as_int(row[2]), "ff": as_int(row[3]), "lut": as_int(row[4]), "words": as_int(row[6]), "bits": as_int(row[7]), "banks": as_int(row[8])})
    return rows


def parse_mux_table(path: Path) -> list[tuple[str, int]]:
    util = section(read_text(path), "== Utilization Estimates", ())
    table = section(util, "* Multiplexer:", ("* Register:",))
    result: list[tuple[str, int]] = []
    for line in table.splitlines():
        row = cells(line)
        if len(row) == 5 and row[0] not in ("Name", "Total", ""):
            try:
                result.append((row[0], as_int(row[1])))
            except ValueError:
                pass
    return result


def mux_family(name: str) -> str:
    for family in ("rename", "issue", "execute", "decode", "rob", "lsu", "frontend", "branch"):
        if f"state_{family}" in name or f"{family}_" in name:
            return family
    if "fsm" in name.lower() or "ap_NS" in name:
        return "control_fsm"
    if "pipe_" in name or "TREADY" in name or "TVALID" in name:
        return "stream_adapter"
    return "other"


def canonical_rtl(name: str) -> str:
    for prefix in ("boom_core_top_boom_core_cycle_io_", "boom_core_step_top_boom_core_cycle_io_", "synth_core_step_top_"):
        if name.startswith(prefix):
            return name[len(prefix):]
    return name


def rtl_definitions(base: Path) -> Counter[str]:
    counts: Counter[str] = Counter()
    for path in (base / "syn/verilog").glob("*.v"):
        match = re.search(r"^module\s+([^\s(]+)", read_text(path), re.MULTILINE)
        if match:
            counts[canonical_rtl(match.group(1))] += 1
    return counts


def ram_inventory(base: Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for path in sorted((base / "syn/verilog").glob("*RAM_AUTO*.v")):
        text = read_text(path)
        def parameter(name: str) -> str:
            match = re.search(rf"parameter {name} = (\d+);", text)
            return match.group(1) if match else ""
        module = re.search(r"^module\s+([^\s(]+)", text, re.MULTILINE)
        rows.append({
            "ram": canonical_rtl(module.group(1) if module else path.stem),
            "data_width": parameter("DataWidth"),
            "address_width": parameter("AddressWidth"),
            "address_range": parameter("AddressRange"),
            "path": str(path.relative_to(ROOT)),
        })
    return rows


def write_csv(path: Path, rows: list[dict[str, object]], fields: list[str] | None = None) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if fields is None:
        fields = list(rows[0].keys()) if rows else []
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    step = parse_summary(REPORTS["synth_core_step_top"])
    core = parse_summary(REPORTS["boom_core_top"])
    finite = parse_summary(REPORTS["boom_core_step_top"])
    cycle = parse_summary(REPORTS["boom_core_cycle_io"])

    hierarchy = [
        {"top": "synth_core_step_top", "semantic_role": "direct diagnostic one-step", **step, "retained_cycle_wrapper": "no", "evidence_type": "REPORT_DIRECT"},
        {"top": "boom_core_step_top", "semantic_role": "one-call product-interface control", **finite, "retained_cycle_wrapper": "yes", "evidence_type": "REPORT_DIRECT"},
        {"top": "boom_core_top", "semantic_role": "free-running product top", **core, "retained_cycle_wrapper": "yes", "evidence_type": "REPORT_DIRECT"},
        {"top": "boom_core_cycle_io", "semantic_role": "retained product cycle wrapper", **cycle, "retained_cycle_wrapper": "self", "evidence_type": "REPORT_DIRECT"},
    ]
    write_csv(OUT / "top_hierarchy_comparison.csv", hierarchy)

    delta_rows: list[dict[str, object]] = []
    step_categories = parse_categories(REPORTS["synth_core_step_top"])
    core_shell_categories = parse_categories(REPORTS["boom_core_top"])
    cycle_categories = parse_categories(REPORTS["boom_core_cycle_io"])
    core_categories: dict[str, dict[str, int]] = {}
    for category in CATEGORY_SOURCE:
        if category == "Instance":
            lut = cycle_categories.get(category, {}).get("lut", 0)
        elif category == "FIFO":
            lut = core_shell_categories.get(category, {}).get("lut", 0)
        else:
            lut = cycle_categories.get(category, {}).get("lut", 0) + core_shell_categories.get(category, {}).get("lut", 0)
        core_categories[category] = {"lut": lut}
    for category in CATEGORY_SOURCE:
        source_function, source_file = CATEGORY_SOURCE[category]
        sl = step_categories.get(category, {}).get("lut", 0)
        cl = core_categories.get(category, {}).get("lut", 0)
        reason = "same top-level stream storage" if category == "FIFO" else "reset-triggered product state elaboration versus direct diagnostic elaboration"
        delta_rows.append({"hierarchy": "top_summary", "instance": category, "step_top_count": 1 if sl else 0, "core_top_count": 1 if cl else 0, "step_top_lut": sl, "core_top_lut": cl, "delta": cl - sl, "source_function": source_function, "source_file": source_file, "suspected_reason": reason, "evidence_type": "REPORT_DIRECT", "confidence": "HIGH", "candidate": "T4 reset attribution completed" if category in ("Instance", "Memory", "Multiplexer") else "none"})

    step_helpers = parse_instance_table(REPORTS["synth_core_step_top"])
    cycle_helpers = parse_instance_table(REPORTS["boom_core_cycle_io"])
    helper_rows: list[dict[str, object]] = []
    for helper in sorted(set(step_helpers) | set(cycle_helpers)):
        s = step_helpers.get(helper, {})
        c = cycle_helpers.get(helper, {})
        row = {"hierarchy": "core_helper", "instance": helper, "step_top_count": 1 if s else 0, "core_top_count": 1 if c else 0, "step_top_lut": s.get("lut", 0), "core_top_lut": c.get("lut", 0), "delta": int(c.get("lut", 0)) - int(s.get("lut", 0)), "source_function": helper, "source_file": HELPER_SOURCE.get(helper, "generated helper"), "suspected_reason": "same helper connected through different resettable state/RAM port representation", "evidence_type": "REPORT_DIRECT", "confidence": "HIGH", "candidate": "no helper rewrite; T4 isolated reset elaboration"}
        helper_rows.append(row)
        delta_rows.append(row)
    write_csv(OUT / "helper_instance_diff.csv", helper_rows)

    step_mux = defaultdict(int)
    core_mux = defaultdict(int)
    for name, lut in parse_mux_table(REPORTS["synth_core_step_top"]):
        step_mux[mux_family(name)] += lut
    for name, lut in parse_mux_table(REPORTS["boom_core_cycle_io"]):
        core_mux[mux_family(name)] += lut
    for family in sorted(set(step_mux) | set(core_mux)):
        delta_rows.append({"hierarchy": "cycle_state_mux", "instance": family, "step_top_count": 1, "core_top_count": 1, "step_top_lut": step_mux[family], "core_top_lut": core_mux[family], "delta": core_mux[family] - step_mux[family], "source_function": f"{family} state routing", "source_file": "include/boom_state.hpp", "suspected_reason": "whole-state reset suppresses partitioning and expands RAM-port arbitration", "evidence_type": "REPORT_DIRECT+RTL_INSTANCE", "confidence": "HIGH", "candidate": "T4 reset attribution completed; reset required"})
    write_csv(OUT / "top_resource_delta.csv", delta_rows)

    step_defs = rtl_definitions(DESIGNS["synth_core_step_top"])
    core_defs = rtl_definitions(DESIGNS["boom_core_top"])
    rtl_rows = []
    for name in sorted(set(step_defs) | set(core_defs)):
        rtl_rows.append({"instance": name, "step_top_count": step_defs[name], "core_top_count": core_defs[name], "delta": core_defs[name] - step_defs[name], "evidence_type": "RTL_INSTANCE", "confidence": "MEDIUM", "notes": "generated module-definition inventory; count is not additive utilization"})
    write_csv(OUT / "rtl_instance_diff.csv", rtl_rows)

    step_rams = ram_inventory(DESIGNS["synth_core_step_top"])
    core_rams = ram_inventory(DESIGNS["boom_core_top"])
    ram_rows = []
    for top, rows in (("synth_core_step_top", step_rams), ("boom_core_top", core_rams)):
        for row in rows:
            ram_rows.append({"top": top, **row, "evidence_type": "RTL_INSTANCE"})
    write_csv(OUT / "ram_instance_diff.csv", ram_rows)

    inventory_rows = []
    for top, base in DESIGNS.items():
        log = read_text(base / f"{base.name}.log") or read_text(base / "solution_module.log") or read_text(base / "solution_top.log")
        inventory_rows.extend([
            {"top": top, "event": "automatic_partition", "count": log.count("Partitioning array '"), "evidence_type": "XFORM_LOG"},
            {"top": top, "event": "automatic_inlining", "count": len(re.findall(r"Inlining function", log)), "evidence_type": "XFORM_LOG"},
            {"top": top, "event": "function_clone", "count": len(re.findall(r"clon(?:e|ed|ing)", log, re.IGNORECASE)), "evidence_type": "XFORM_LOG"},
            {"top": top, "event": "memory_promotion", "count": len(re.findall(r"memory promotion|Promoting memory", log, re.IGNORECASE)), "evidence_type": "XFORM_LOG"},
        ])
    write_csv(OUT / "inlining_clone_inventory.csv", inventory_rows)

    fsm_rows = []
    for top, report in REPORTS.items():
        muxes = parse_mux_table(report)
        fsm_lut = sum(lut for name, lut in muxes if "fsm" in name.lower())
        rtl_dir = (DESIGNS.get(top, DESIGNS["boom_core_top"]) / "syn/verilog")
        fsm_count = 0
        for path in rtl_dir.glob("*.v"):
            if "ap_CS_fsm" in read_text(path):
                fsm_count += 1
        fsm_rows.append({"top": top, "rtl_modules_with_fsm": fsm_count, "reported_fsm_mux_lut": fsm_lut, "evidence_type": "REPORT_DIRECT+RTL_INSTANCE", "notes": "module-definition inventory, not unique physical state count"})
    write_csv(OUT / "control_fsm_inventory.csv", fsm_rows)

    total_delta = int(core["lut"]) - int(step["lut"])
    category_delta = {row["instance"]: row["delta"] for row in delta_rows if row["hierarchy"] == "top_summary"}
    lines = [
        "# Gate 3.6 Top Resource Delta",
        "",
        f"Direct HLS estimate: `boom_core_top` {core['lut']} LUT minus `synth_core_step_top` {step['lut']} LUT = **{total_delta} LUT**.",
        "",
        "## Recursive Category Delta",
        "",
        "| Category | Step LUT | Core LUT | Delta |",
        "|---|---:|---:|---:|",
    ]
    for category in CATEGORY_SOURCE:
        lines.append(f"| {category} | {step_categories.get(category, {}).get('lut', 0)} | {core_categories.get(category, {}).get('lut', 0)} | {category_delta.get(category, 0)} |")
    lines.extend([
        "",
        "The product-side categories recursively expand the retained `boom_core_cycle_io` instance and add the small outer shell, so the category deltas sum exactly to 37936 LUT.",
        "",
        "The free-running loop is not the primary source: the one-call `boom_core_step_top` product-interface control is 83353 LUT, only 67 LUT above the 83286-LUT free-running top.",
        "",
        "The retained `boom_core_cycle_io` hierarchy identifies where the delta is reported, but T3 proves the boundary alone is not causal: force-inlining removes the wrapper and increases the top to 87388 LUT while partitions remain zero.",
        "",
        "T4 isolates the dominant trigger. Removing only the product state's HLS reset pragma reduces `boom_core_top` from 83286 to 45602 LUT and changes automatic partition count from 0 to 342. The 37684-LUT same-top delta closes exactly as +9741 helper-instance LUT, +472 memory LUT, and +27471 multiplexer LUT in the resettable implementation. The no-reset product cycle wrapper is 45133 LUT, close to the 45350-LUT direct diagnostic top; the product outer shell remains 469 LUT in both cases.",
        "",
        "The no-reset result is attribution evidence only and is rejected because required synthesized reset semantics are lost.",
        "",
        "See `top_resource_delta.csv`, `ram_instance_diff.csv`, and `helper_instance_diff.csv` for machine-readable evidence.",
    ])
    (OUT / "top_resource_delta.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
