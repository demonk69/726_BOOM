#!/usr/bin/env python3
import csv
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports/gate4_0/w1"
XML = OUT / "csynth/boom_core_top_csynth.xml"
BASELINE = {"period_ns": 5.898, "lut": 47999, "ff": 12134, "bram_18k": 12, "dsp": 3}


def value(root, path, cast):
    node = root.find(path)
    if node is None or node.text is None:
        raise SystemExit(f"missing synthesis field: {path}")
    return cast(node.text)


def main():
    if not XML.is_file():
        raise SystemExit(f"missing W1 synthesis report: {XML}")
    root = ET.parse(XML).getroot()
    current = {
        "period_ns": value(root, "./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod", float),
        "lut": value(root, "./AreaEstimates/Resources/LUT", int),
        "ff": value(root, "./AreaEstimates/Resources/FF", int),
        "bram_18k": value(root, "./AreaEstimates/Resources/BRAM_18K", int),
        "dsp": value(root, "./AreaEstimates/Resources/DSP", int),
    }
    trace = (OUT / "regression/trace_diff.md").read_text()
    arch = (OUT / "regression/full_program_architectural_diff.md").read_text()
    reset = (OUT / "regression/logs/reset_architecture_tests.log").read_text()
    lane = (OUT / "regression/logs/gate4_w1_lane_interface_tests.log").read_text()
    checks = {
        "trace": "Result: 10/10 byte-identical" in trace,
        "architecture": "Result: 10/10 PASS" in arch,
        "reset": "=== 14 passed, 0 failed ===" in reset,
        "lane_interface": "Gate 4.0 W1 lane interface tests: PASS" in lane,
    }
    if not all(checks.values()):
        raise SystemExit(f"W1 verification incomplete: {checks}")

    with (OUT / "w1_resource_summary.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["variant", "period_ns", "lut", "ff", "bram_18k", "dsp",
                         "lut_delta", "ff_delta", "bram_delta", "dsp_delta", "core_cycle_pipeline"])
        writer.writerow(["GATE3_9_ACCEPTED", BASELINE["period_ns"], BASELINE["lut"], BASELINE["ff"],
                         BASELINE["bram_18k"], BASELINE["dsp"], 0, 0, 0, 0, "no"])
        writer.writerow(["GATE4_0_W1", current["period_ns"], current["lut"], current["ff"],
                         current["bram_18k"], current["dsp"],
                         current["lut"] - BASELINE["lut"], current["ff"] - BASELINE["ff"],
                         current["bram_18k"] - BASELINE["bram_18k"],
                         current["dsp"] - BASELINE["dsp"], "no"])

    report = f"""# Gate 4.0 W1 Results

Status: `W1_FIXED_LANE_INTERFACE_VERIFIED`.

## Change

The issue interface remains `ISSUE_WIDTH=3`, and the execute-result interface now has the same fixed lane count. The implemented issue/result cap remains `DISPATCH_WIDTH=1`, so only lane 0 can become valid in W1. Branch recovery, LSU intake, ROB completion, and reset now consume or clear every fixed result lane.

## Verification

- Dedicated lane interface test: PASS; lanes 1 and 2 remain invalid and the unissued IQ entry is retained.
- Existing directed, IQ, LSU, branch snapshot, and 21-seed randomized branch regressions: PASS.
- Reset architecture: 14/14 PASS.
- Gate 3.9 frozen C++ and C-sim traces: 10/10 byte-identical.
- Full-program C++ and C-sim architectural checks: 10/10 PASS.
- Conservative top-level synthesis: PASS; `CORE_CYCLE` remains unpipelined.

## Synthesis

| Variant | Estimated period | LUT | FF | BRAM_18K | DSP |
|---|---:|---:|---:|---:|---:|
| Gate 3.9 accepted | {BASELINE['period_ns']:.3f} ns | {BASELINE['lut']} | {BASELINE['ff']} | {BASELINE['bram_18k']} | {BASELINE['dsp']} |
| Gate 4.0 W1 | {current['period_ns']:.3f} ns | {current['lut']} | {current['ff']} | {current['bram_18k']} | {current['dsp']} |

W1 adds {current['lut'] - BASELINE['lut']} LUT and {current['ff'] - BASELINE['ff']} FF while preserving the 5.898 ns estimate, 12 BRAM_18K, and 3 DSP. This is an interface-expansion checkpoint, not an accepted performance configuration.

## Decision

W1 passes its functional and synthesis gate. W2 may experiment with at most two simultaneous grants, constrained to one MEM-compatible uop and one INT-compatible uop. The FP queue remains out of scope, so peak integer issue 3 remains prohibited.
"""
    (OUT / "w1_results.md").write_text(report)


if __name__ == "__main__":
    main()
