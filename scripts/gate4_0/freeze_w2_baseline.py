#!/usr/bin/env python3
"""Freeze the Gate 4.0 W2 baseline from the immutable W1 commit."""

import csv
import hashlib
import io
import os
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports/gate4_0/w2"
W1_REF = "fa3dbcc"
W1 = "reports/gate4_0/w1"
G39 = "reports/gate3_9"
SELF = "scripts/gate4_0/freeze_w2_baseline.py"
OUTPUT_NAMES = {
    "baseline_manifest.md",
    "source_hashes_before.txt",
    "regression_before.md",
    "w1_resource_baseline.csv",
    "w1_trace_manifest.csv",
}


def git(*args, binary=False):
    result = subprocess.run(
        ["git", *args], cwd=ROOT, check=True, capture_output=True
    )
    return result.stdout if binary else result.stdout.decode("utf-8")


def blob(commit, path):
    try:
        return git("show", f"{commit}:{path}", binary=True)
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f"missing committed baseline artifact: {path}") from exc


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def csv_rows(data):
    return list(csv.DictReader(io.StringIO(data.decode("utf-8"))))


def write_atomic(name, data):
    OUT.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", dir=OUT, delete=False, newline="") as handle:
        handle.write(data)
        temporary = Path(handle.name)
    os.replace(temporary, OUT / name)


def classify_dirty():
    groups = {}
    entries = git("status", "--porcelain=v1", "-z", "--untracked-files=normal").split("\0")
    for entry in entries:
        if not entry:
            continue
        status, path = entry[:2], entry[3:]
        if " -> " in path:
            path = path.split(" -> ", 1)[1]
        if path == SELF or path.startswith("reports/gate4_0/w2/"):
            continue
        if status == "??":
            if path == "build/" or path.startswith("build/"):
                group = "untracked build tree"
            elif path.endswith(".backup.log"):
                group = "untracked backup logs"
            elif path.startswith("reports/"):
                group = "untracked generated report binaries/artifacts"
            else:
                group = "other untracked paths"
        elif path.endswith(".log"):
            group = "modified tracked logs"
        else:
            group = "other tracked changes"
        groups[group] = groups.get(group, 0) + 1
    return groups


def source_hashes(commit):
    paths = git("ls-tree", "-r", "--name-only", commit, "src", "include").splitlines()
    paths = sorted(
        path for path in paths
        if Path(path).suffix in {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}
    )
    if not paths:
        raise SystemExit("no W1 production source files found")
    return "".join(f"{sha256(blob(commit, path))}  {path}\n" for path in paths)


def resource_csv(commit):
    xml_path = f"{W1}/csynth/boom_core_top_csynth.xml"
    rpt_path = f"{W1}/csynth/boom_core_top_csynth.rpt"
    time_path = f"{W1}/csynth/csynth.time"
    xml_data, rpt_data, time_data = (
        blob(commit, xml_path), blob(commit, rpt_path), blob(commit, time_path)
    )
    root = ET.fromstring(xml_data)

    def field(path, cast):
        node = root.find(path)
        if node is None or node.text is None:
            raise SystemExit(f"missing W1 csynth XML field: {path}")
        return cast(node.text)

    timing = field(
        "./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod", float
    )
    resources = {
        name: field(f"./AreaEstimates/Resources/{name}", int)
        for name in ("LUT", "FF", "BRAM_18K", "DSP")
    }
    report = rpt_data.decode("utf-8")
    if not re.search(r"\|[- ]*CORE_CYCLE.*\|\s*no\|", report):
        raise SystemExit("W1 csynth report does not show CORE_CYCLE as unpipelined")
    timing_values = dict(
        line.split("=", 1) for line in time_data.decode("utf-8").splitlines() if "=" in line
    )
    handle = io.StringIO(newline="")
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow([
        "baseline", "commit", "period_ns", "lut", "ff", "bram_18k", "dsp",
        "core_cycle_pipeline", "runtime_seconds", "peak_memory_kb", "xml_path",
        "xml_sha256", "report_path", "report_sha256",
    ])
    writer.writerow([
        "GATE4_0_W1", commit, f"{timing:.3f}", resources["LUT"], resources["FF"],
        resources["BRAM_18K"], resources["DSP"], "no",
        timing_values.get("runtime_seconds", ""), timing_values.get("peak_memory_kb", ""),
        xml_path, sha256(xml_data), rpt_path, sha256(rpt_data),
    ])
    return handle.getvalue(), resources, timing


def trace_csv(commit):
    trace_diff_path = f"{W1}/regression/trace_diff.md"
    trace_diff = blob(commit, trace_diff_path).decode("utf-8")
    if "Result: 10/10 byte-identical" not in trace_diff:
        raise SystemExit("W1 frozen trace comparison is not 10/10 byte-identical")
    compared = {
        (row[0], row[1])
        for row in re.findall(r"\| ([a-z_]+) \| (hls_(?:cpp|csim)) \| PASS \|", trace_diff)
    }

    rows = []
    hls_prefix = f"{W1}/regression/hls_traces/"
    hls_paths = sorted(
        path for path in git("ls-tree", "-r", "--name-only", commit, hls_prefix).splitlines()
        if path.endswith(".jsonl")
    )
    for path in hls_paths:
        match = re.fullmatch(r"(.+)_hls_(cpp|csim)_full\.jsonl", Path(path).name)
        if not match:
            raise SystemExit(f"unexpected W1 trace name: {path}")
        program, kind = match.groups()
        producer = f"hls_{kind}"
        data = blob(commit, path)
        rows.append([
            "GATE4_0_W1", producer, "", program, "PASS",
            "PASS_BYTE_IDENTICAL" if (program, producer) in compared else "NOT_IN_FROZEN_10_TRACE_SET",
            path, sha256(data), len(data), len(data.splitlines()), commit,
        ])

    matrix_path = f"{G39}/rtl_test_matrix.csv"
    matrix_data = blob(commit, matrix_path)
    matrix = csv_rows(matrix_data)
    if len(matrix) != 49 or any(row["status"] != "PASS" for row in matrix):
        raise SystemExit("Gate 3.9 RTL baseline is not 49/49 PASS")
    for row in matrix:
        path = row["trace"]
        data = blob(commit, path)
        rows.append([
            "GATE3_9", "rtl_xsim", row["test"], row["program"], row["status"],
            "ARCHITECTURAL_PASS", path, sha256(data), len(data), len(data.splitlines()), commit,
        ])
    if len(hls_paths) != 14 or len(rows) != 63:
        raise SystemExit(f"unexpected trace inventory: W1={len(hls_paths)}, total={len(rows)}")

    handle = io.StringIO(newline="")
    writer = csv.writer(handle, lineterminator="\n")
    writer.writerow([
        "gate", "producer", "test", "program", "outcome", "comparison",
        "path", "sha256", "bytes", "records", "commit",
    ])
    writer.writerows(rows)
    return handle.getvalue(), len(hls_paths), len(matrix), sha256(matrix_data)


def regression_report(commit):
    suites = [
        ("Directed", "directed_tests.log", 25, 1),
        ("Gate 1 regression", "gate1_regression_tests.log", 13, 1),
        ("IQ compaction", "iq_compaction_tests.log", 10, 1),
        ("Branch snapshot", "branch_snapshot_tests.log", 30, 1),
        ("Branch randomized", "branch_snapshot_random_tests.log", 42, 21),
        ("Minimal LSU", "lsu_minimal_tests.log", 14, 1),
        ("Reset architecture", "reset_architecture_tests.log", 14, 1),
    ]
    lines = [
        "# Gate 4.0 W2 Regression Baseline", "",
        f"Immutable W1 commit: `{commit}`.", "",
        "No tests were rerun for this freeze; outcomes below are parsed from committed W1 evidence.", "",
        "| Suite | Outcome | Passed | Failed | Runs | Evidence SHA-256 |", "|---|---|---:|---:|---:|---|",
    ]
    total = 0
    for label, name, expected, runs in suites:
        path = f"{W1}/regression/logs/{name}"
        data = blob(commit, path)
        summaries = re.findall(rb"=== (\d+) passed, (\d+) failed ===", data)
        passed = sum(int(item[0]) for item in summaries)
        failed = sum(int(item[1]) for item in summaries)
        if passed != expected or failed or len(summaries) != runs:
            raise SystemExit(f"unexpected W1 test outcome in {path}")
        total += passed
        lines.append(f"| {label} | PASS | {passed} | {failed} | {runs} | `{sha256(data)}` |")

    lane_path = f"{W1}/regression/logs/gate4_w1_lane_interface_tests.log"
    lane_data = blob(commit, lane_path)
    if b"Gate 4.0 W1 lane interface tests: PASS" not in lane_data:
        raise SystemExit("W1 lane interface test did not pass")
    lines.append(f"| W1 lane interface | PASS | 1 | 0 | 1 | `{sha256(lane_data)}` |")
    total += 1

    trace_path = f"{W1}/regression/trace_diff.md"
    arch_path = f"{W1}/regression/full_program_architectural_diff.csv"
    trace_data, arch_data = blob(commit, trace_path), blob(commit, arch_path)
    arch = csv_rows(arch_data)
    if b"Result: 10/10 byte-identical" not in trace_data or len(arch) != 10 or any(
        row["status"] != "PASS" for row in arch
    ):
        raise SystemExit("W1 trace or architectural regression evidence is incomplete")
    lines.extend([
        "", "## Trace And Synthesis Outcomes", "",
        f"- W1 frozen C++/csim trace comparison: **PASS**, 10/10 byte-identical; evidence `{trace_path}`, SHA-256 `{sha256(trace_data)}`.",
        f"- W1 full-program architectural diff: **PASS**, 10/10; evidence `{arch_path}`, SHA-256 `{sha256(arch_data)}`.",
        "- W1 conservative csynth: **PASS**; resources and report identities are frozen in `w1_resource_baseline.csv`.",
        "", "## Summary", "",
        f"Recorded unit/regression assertions: **{total} passed, 0 failed**. Randomized branch coverage comprises 21 runs and 42 passing assertions.",
        "",
    ])
    return "\n".join(lines), total


def manifest(commit, head, dirty, resources, period, hls_count, rtl_count, matrix_sha, tests):
    dirty_lines = [
        f"- {name}: {count} porcelain status entr{'y' if count == 1 else 'ies'}"
        for name, count in sorted(dirty.items())
    ] or ["- clean (excluding this generator and its five W2 outputs)"]
    return "\n".join([
        "# Gate 4.0 W2 Baseline Freeze", "",
        f"- W1 reference: `{W1_REF}`",
        f"- Resolved immutable W1 commit: `{commit}`",
        f"- Current commit at freeze: `{head}`",
        "- Baseline source: committed blobs from the immutable W1 commit, never worktree copies",
        "", "## Frozen Baseline", "",
        f"- Production C/C++ source identities: `source_hashes_before.txt`.",
        f"- W1 csynth: {period:.3f} ns, {resources['LUT']} LUT, {resources['FF']} FF, {resources['BRAM_18K']} BRAM_18K, {resources['DSP']} DSP; `CORE_CYCLE` unpipelined.",
        f"- W1 traces: {hls_count} total, comprising seven C++ and seven csim traces; the frozen five-program pair is 10/10 byte-identical.",
        f"- Gate 3.9 RTL traces: {rtl_count}/49 PASS; matrix SHA-256 `{matrix_sha}`.",
        f"- W1 unit/regression assertions: {tests} passed, 0 failed; architectural checks 10/10 PASS.",
        "", "## Pre-existing Dirty Worktree Groups", "",
        *dirty_lines,
        "",
        "Dirty paths are grouped by type only. Unrelated log names and contents are intentionally not copied into this freeze, and no pre-existing path is modified.",
        "", "## Artifacts", "",
        "- `baseline_manifest.md`: commit, baseline summary, and grouped pre-existing worktree state.",
        "- `source_hashes_before.txt`: SHA-256 identities of committed production sources under `src/` and `include/`.",
        "- `regression_before.md`: committed W1 test, trace, architectural, and csynth outcomes.",
        "- `w1_resource_baseline.csv`: W1 csynth resources plus immutable report identities.",
        "- `w1_trace_manifest.csv`: immutable identities for W1 C++/csim and Gate 3.9 RTL traces.",
        "", "## Reproduction", "",
        "Run `python3 scripts/gate4_0/freeze_w2_baseline.py` from anywhere in this repository. The script writes only these five W2 files and excludes them and itself from dirty-state grouping.",
        "", "## Limitations", "",
        "- This is an evidence freeze, not a rerun of tests, csim, synthesis, or RTL simulation.",
        "- W1 `load_store` and `tohost` C++/csim traces are inventoried but were not members of the committed 10-trace byte-comparison set.",
        "- Official Chipyard/FESVR/DRAMSim equivalence remained unavailable in W1; the recorded full-program result uses the committed HLS architectural comparison.",
        "",
    ])


def main():
    os.chdir(ROOT)
    commit = git("rev-parse", f"{W1_REF}^{{commit}}").strip()
    head = git("rev-parse", "HEAD").strip()
    dirty = classify_dirty()
    sources = source_hashes(commit)
    resources_csv, resources, period = resource_csv(commit)
    traces_csv, hls_count, rtl_count, matrix_sha = trace_csv(commit)
    regression, tests = regression_report(commit)
    baseline = manifest(
        commit, head, dirty, resources, period, hls_count, rtl_count, matrix_sha, tests
    )
    write_atomic("source_hashes_before.txt", sources)
    write_atomic("w1_resource_baseline.csv", resources_csv)
    write_atomic("w1_trace_manifest.csv", traces_csv)
    write_atomic("regression_before.md", regression)
    write_atomic("baseline_manifest.md", baseline)


if __name__ == "__main__":
    main()
