#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports/gate4_0"
DEFAULT_GENERATED = ROOT.parent / "chipyard.TestHarness.SmallBoomConfig"
BASELINE_COMMIT = "736926352950b084d789ffca8317673c01e395e6"
ACCEPTED_COMMIT = "557bdf55fe4798096b4bd6be68e50c72f8b1e07e"


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def git_digest(commit, relative_path):
    data = subprocess.check_output(
        ["git", "show", f"{commit}:{relative_path}"], cwd=ROOT
    )
    return hashlib.sha256(data).hexdigest()


def find_issue_params(value):
    found = []
    if isinstance(value, dict):
        params = value.get("issueParams")
        if isinstance(params, list):
            normalized = tuple(
                (item["dispatchWidth"], item["issueWidth"], item["numEntries"], item["iqType"])
                for item in params
            )
            found.append(normalized)
        for child in value.values():
            found.extend(find_issue_params(child))
    elif isinstance(value, list):
        for child in value:
            found.extend(find_issue_params(child))
    return found


def module_body(verilog, module_name):
    match = re.search(
        rf"^module {re.escape(module_name)}\((.*?)^endmodule$",
        verilog,
        flags=re.MULTILINE | re.DOTALL,
    )
    if not match:
        raise SystemExit(f"missing module {module_name}")
    return match.group(1)


def count_indexed_ports(body, stem):
    return len(set(re.findall(rf"\b{re.escape(stem)}_(\d+)_", body)))


def require(text, pattern, description):
    if not re.search(pattern, text, flags=re.MULTILINE):
        raise SystemExit(f"missing topology evidence: {description}")


def write_csv(path, header, rows):
    with path.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(header)
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--generated-dir", type=Path, default=DEFAULT_GENERATED)
    args = parser.parse_args()

    generated = args.generated_dir.resolve()
    anno = generated / "chipyard.TestHarness.SmallBoomConfig.anno.json"
    fir = generated / "chipyard.TestHarness.SmallBoomConfig.fir"
    verilog_path = generated / "chipyard.TestHarness.SmallBoomConfig.top.v"
    for path in (anno, fir, verilog_path):
        if not path.is_file():
            raise SystemExit(f"missing generated SmallBoom artifact: {path}")

    params_sets = set(find_issue_params(json.loads(anno.read_text())))
    if len(params_sets) != 1:
        raise SystemExit(f"expected one unique issueParams topology, found {len(params_sets)}")
    params = next(iter(params_sets))
    expected = ((1, 1, 8, 2), (1, 1, 8, 1), (1, 1, 8, 4))
    if params != expected:
        raise SystemExit(f"unexpected SmallBoom issueParams: {params}")

    verilog = verilog_path.read_text()
    evidence = {
        "MEM": (
            r"IssueUnitCollapsing_1 mem_issue_unit",
            r"dispatcher_io_dis_uops_0_0_ready = mem_issue_unit_io_dis_uops_0_ready",
            r"ALUExeUnit mem_units_0",
        ),
        "INT": (
            r"IssueUnitCollapsing_2 int_issue_unit",
            r"dispatcher_io_dis_uops_1_0_ready = int_issue_unit_io_dis_uops_0_ready",
            r"ALUExeUnit_1 csr_exe_unit",
        ),
        "FP": (
            r"IssueUnitCollapsing fp_issue_unit",
            r"dispatcher_io_dis_uops_2_0_ready = fp_pipeline_io_dis_uops_0_ready",
            r"FpPipeline fp_pipeline",
        ),
    }
    for queue, patterns in evidence.items():
        for pattern in patterns:
            require(verilog, pattern, f"{queue}: {pattern}")

    require(verilog, r"assign mem_issue_unit_io_fu_types_0 = .*10'h4", "MEM FU mask")
    require(verilog, r"assign int_issue_unit_io_fu_types_0 = csr_exe_unit_io_fu_types", "INT FU mask")
    int_exe = module_body(verilog, "ALUExeUnit_1")
    for unit in ("ALUUnit alu", "PipelinedMulUnit imul", "DivUnit div"):
        require(int_exe, unit, f"INT execution unit {unit}")

    ireg = module_body(verilog, "RegisterFileSynthesizable_1")
    read_ports = count_indexed_ports(ireg, "io_read_ports")
    write_ports = count_indexed_ports(ireg, "io_write_ports")
    depth_match = re.search(r"reg \[63:0\] regfile \[0:(\d+)\]", ireg)
    if not depth_match:
        raise SystemExit("could not determine integer PRF depth")
    depth = int(depth_match.group(1)) + 1
    if (depth, read_ports, write_ports) != (52, 4, 2):
        raise SystemExit(f"unexpected integer PRF topology: {(depth, read_ports, write_ports)}")

    OUT.mkdir(parents=True, exist_ok=True)
    labels = ("MEM", "INT", "FP")
    instances = ("mem_issue_unit", "int_issue_unit", "fp_issue_unit")
    modules = ("IssueUnitCollapsing_1", "IssueUnitCollapsing_2", "IssueUnitCollapsing")
    topology_rows = []
    for index, ((dispatch, issue, entries, iq_type), label, instance, module) in enumerate(
        zip(params, labels, instances, modules)
    ):
        topology_rows.append(
            [index, label, iq_type, dispatch, issue, entries, instance, module,
             "yes" if label != "FP" else "no"]
        )
    write_csv(
        OUT / "execution_topology.csv",
        ["config_index", "queue", "iq_type_mask", "dispatch_width", "issue_width",
         "entries", "rtl_instance", "rtl_module", "in_integer_only_scope"],
        topology_rows,
    )

    write_csv(
        OUT / "fu_port_mapping.csv",
        ["queue", "issue_lanes", "consumer", "fu_classes", "integer_only_scope", "evidence"],
        [
            ["MEM", 1, "mem_units_0 (ALUExeUnit)", "memory address generation/LSU", "yes",
             "top.v: mem_issue_unit fu_types=10'h4 and mem_units_0 instance"],
            ["INT", 1, "csr_exe_unit (ALUExeUnit_1)", "ALU/branch/CSR/multiply/divide/int-to-FP", "yes",
             "top.v: int_issue_unit uses csr_exe_unit_io_fu_types"],
            ["FP", 1, "fp_pipeline (FpPipeline)", "floating point", "no",
             "top.v: dispatcher queue 2 connects to fp_pipeline"],
        ],
    )

    write_csv(
        OUT / "prf_port_inventory.csv",
        ["register_file", "entries", "data_bits", "read_ports", "write_ports", "rtl_module", "rtl_instance"],
        [["integer", depth, 64, read_ports, write_ports, "RegisterFileSynthesizable_1", "iregfile"]],
    )

    tracked = [
        "include/boom_config.hpp", "include/boom_state.hpp", "include/boom_types.hpp",
        "src/issue.cpp", "src/execute.cpp", "src/rob.cpp", "src/branch.cpp",
        "src/lsu.cpp", "src/boom_core_step.cpp", "scripts/generate_merged.sh",
    ]
    with (OUT / "baseline_source_hashes.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["commit", "path", "sha256"])
        for path in tracked:
            writer.writerow([BASELINE_COMMIT, path, git_digest(BASELINE_COMMIT, path)])

    with (OUT / "reference_artifact_hashes.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["path", "sha256"])
        for path in (anno, fir, verilog_path):
            writer.writerow([str(path), digest(path)])

    result = f"""# Gate 4.0 W0 Topology Freeze

Status: `W0_TOPOLOGY_FROZEN`.

## Baseline

- Gate 4.0 starting commit: `{BASELINE_COMMIT}`.
- Accepted functional configuration: Gate 3.9 commit `{ACCEPTED_COMMIT}`.
- Gate 3.9 and Gate 3.10 evidence remains referenced in place and is not copied or overwritten.

## Extracted SmallBoom Topology

- The generated configuration has three issue queues, each with dispatch width 1, issue width 1, and 8 entries.
- Dispatcher queue 0 is MEM and feeds one memory execution path (`mem_units_0`).
- Dispatcher queue 1 is INT and feeds one general integer path (`csr_exe_unit`) containing ALU/branch/CSR, multiply, and divide functionality.
- Dispatcher queue 2 feeds `FpPipeline`; it is not a third integer ALU path.
- The integer PRF has {depth} entries, {read_ports} read ports, and {write_ports} write ports.

## Gate 4.0 Scope Decision

With FPU work excluded, the faithful SmallBoom peak is one MEM issue plus one INT issue per cycle. Gate 4.0 must not claim peak integer issue 3. The maximum honest outcome is `PARTIAL_WIDE_ISSUE_MAX2`; W1 remains single-issue while introducing fixed lane-shaped interfaces.

## Evidence

`execution_topology.csv`, `fu_port_mapping.csv`, and `prf_port_inventory.csv` are regenerated by `scripts/gate4_0/extract_smallboom_topology.py`. The script fails if the expected generated configuration, queue wiring, execution units, or integer PRF ports are absent.
"""
    (OUT / "w0_topology_freeze.md").write_text(result)


if __name__ == "__main__":
    main()
