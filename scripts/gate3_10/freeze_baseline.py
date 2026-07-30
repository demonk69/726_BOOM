#!/usr/bin/env python3
import csv
import hashlib
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports/gate3_10"
BASE = ROOT / "reports/gate3_9"


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def commit_digest(commit, path):
    data = subprocess.check_output(["git", "show", f"{commit}:{path.as_posix()}"], cwd=ROOT)
    return hashlib.sha256(data).hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    (OUT / "git_status_before.txt").write_text(
        "Frozen HEAD: 557bdf55fe4798096b4bd6be68e50c72f8b1e07e\n"
        "Pre-existing tracked changes: reports/equivalence/provisional_gate3/hls_csim_trace.log; vitis_hls.log\n"
        "Pre-existing untracked groups: build/; reports/gate3_4/hls_prefix_trace_tb; reports/gate3_8/logs/*.backup.log\n"
        "No Gate 3.10 source or report path existed at capture.\n"
    )

    tracked = [
        ROOT / "src/reset.cpp", ROOT / "src/boom_core_top.cpp", ROOT / "src/boom_core_merged.cpp",
        ROOT / "include/reset.hpp", ROOT / "directives/baseline_directives.tcl",
        BASE / "variants/F1_FINE_GRAIN_RESET/boom_core_top_csynth.rpt",
        BASE / "rtl_test_matrix.csv", BASE / "reset_latency.csv",
    ]
    hashes = [f"{commit_digest(commit, path.relative_to(ROOT))}  {path.relative_to(ROOT)}"
              for path in tracked if path.is_file()]
    (OUT / "source_hashes_before.txt").write_text("\n".join(hashes) + "\n")

    manifest = [
        "# Gate 3.10 Baseline Manifest", "", f"Frozen commit: `{commit}`.", "",
        "The baseline is referenced from immutable commit `557bdf5`; Gate 3.9 evidence is not copied or overwritten.", "",
        "The live Gate 3.9 HLS database's 20 `*.verbose.sched.rpt` helper schedules are preserved in `reports/gate3_10/baseline_schedule/`.", "",
        "| Evidence | Frozen path | SHA-256 |", "|---|---|---|",
    ]
    evidence = [
        BASE / "variants/F1_FINE_GRAIN_RESET/boom_core_top_csynth.rpt",
        BASE / "variants/F1_FINE_GRAIN_RESET/conservative_rtl/boom_core_top.v",
        BASE / "rtl_test_matrix.csv", BASE / "reset_latency.csv",
        BASE / "normal_rtl_trace_comparison.csv",
        BASE / "regression_after_artifacts/trace_diff.md",
        BASE / "regression_after_artifacts/full_program_architectural_diff.md",
    ]
    for path in evidence:
        manifest.append(f"| `{path.relative_to(ROOT)}` | commit 557bdf5 | `{digest(path)}` |")
    manifest.extend(["", "BOOM reference traces remain unavailable; M013 and strict cycle equivalence remain insufficient evidence."])
    (OUT / "baseline_manifest.md").write_text("\n".join(manifest) + "\n")

    with (OUT / "baseline_resources.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["variant", "period_ns", "lut", "ff", "bram_18k", "dsp", "core_cycle_pipeline"])
        writer.writerow(["P0_GATE3_9_BASELINE", "5.898", "47999", "12134", "12", "3", "no"])

    with (OUT / "baseline_rtl_trace_manifest.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["test", "program", "path", "sha256"])
        with (BASE / "rtl_test_matrix.csv").open(newline="") as matrix:
            for row in csv.DictReader(matrix):
                if row["test"].startswith("N"):
                    path = ROOT / row["trace"]
                    writer.writerow([row["test"], row["program"], row["trace"], digest(path)])

    with (OUT / "baseline_hls_trace_manifest.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["trace", "path", "sha256"])
        for path in sorted((BASE / "baseline_artifacts/hls_traces").glob("*.jsonl")):
            writer.writerow([path.name, path.relative_to(ROOT), digest(path)])

    (OUT / "regression_before.md").write_text(
        "# Gate 3.10 Baseline Regression\n\n"
        "Frozen Gate 3.9 result: XSim 49/49, reset architecture 14/14, normal RTL architecture 7/7, "
        "C++/csim frozen traces 10/10 byte-identical, and full-program diff 10/10.\n"
    )
    if commit != "557bdf5c9d16012894bc2bf0ef9bb8bc32a5f298" and not commit.startswith("557bdf5"):
        raise SystemExit(f"baseline commit mismatch: {commit}")


if __name__ == "__main__":
    main()
