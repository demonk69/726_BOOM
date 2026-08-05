#!/usr/bin/env python3
import csv
import hashlib
from pathlib import Path

root = Path(__file__).resolve().parents[2]
report = root / "reports/gate4_1/m2/m2b"
sources = (
    "include/mul.hpp",
    "src/mul.cpp",
    "src/execute.cpp",
    "src/completion.cpp",
    "src/rob.cpp",
    "src/lsu.cpp",
    "src/synth_module_tops.cpp",
    "src/boom_core_merged.cpp",
    "scripts/gate4_0/run_w3_regressions.sh",
    "scripts/gate4_0/run_w3_rtl.sh",
    "scripts/gate4_0/run_w4e_regressions.sh",
    "scripts/run_hls_prefix_trace_csim.tcl",
    "scripts/gate4_1/run_m2b_tests.sh",
    "scripts/gate4_1/run_m2b_rtl.sh",
    "scripts/gate4_1/run_m2b_csynth.sh",
    "scripts/gate4_1/finalize_m2b_evidence.py",
    "tb/differential/m2b_execute_tests.cpp",
    "tb/differential/m2b_execute_random_tests.cpp",
    "tb/differential/m2b_full_core_tests.cpp",
    "tb/programs/boom_reference/m2b_mul_family.hex",
    "rtl_tb/gate4_1/m2b_execute_tb.sv",
    "docs/gate4_1_m_extension.md",
    "docs/multiply_microarchitecture.md",
)

with (report / "source_hashes_after.txt").open("w", encoding="utf-8") as stream:
    stream.write("# sha256  path\n")
    for name in sources:
        path = root / name
        stream.write(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n")

artifacts = []
for path in sorted(report.rglob("*")):
    if path.is_file() and path.name != "artifact_manifest.csv":
        artifacts.append((str(path.relative_to(report)), path.stat().st_size,
                          hashlib.sha256(path.read_bytes()).hexdigest()))
with (report / "artifact_manifest.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("path", "bytes", "sha256"))
    writer.writerows(artifacts)

print(f"M2B evidence finalized: {len(sources)} sources, {len(artifacts)} artifacts")
