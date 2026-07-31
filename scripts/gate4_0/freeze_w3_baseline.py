#!/usr/bin/env python3
"""Freeze the Gate 4.0 W3 baseline from immutable committed W2 evidence."""

import csv
import hashlib
import io
import os
import re
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports/gate4_0/w3"
W2_COMMIT = "210ad1900457b073806a54617d313a2c61a14e21"
W2 = "reports/gate4_0/w2"
SELF = "scripts/gate4_0/freeze_w3_baseline.py"
OUTPUT_NAMES = {
    "baseline_manifest.md",
    "git_status_before.txt",
    "source_hashes_before.txt",
    "regression_before.md",
    "w2_resource_baseline.csv",
    "w2_trace_manifest.csv",
    "w2_grant_accounting.csv",
}


def git(*args, binary=False):
    result = subprocess.run(
        ["git", *args], cwd=ROOT, check=True, capture_output=True
    )
    return result.stdout if binary else result.stdout.decode("utf-8")


def blob(path):
    try:
        return git("show", f"{W2_COMMIT}:{path}", binary=True)
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f"missing committed W2 artifact: {path}") from exc


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


def grouped_status():
    groups = {}
    for line in git("status", "--porcelain=v1", "--untracked-files=normal").splitlines():
        status, path = line[:2], line[3:]
        if " -> " in path:
            path = path.split(" -> ", 1)[1]
        if path == SELF or path.startswith("reports/gate4_0/w3/"):
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
    lines = [
        f"baseline_commit={W2_COMMIT}",
        "format=aggregate porcelain-v1 counts; paths and backup contents omitted",
    ]
    lines.extend(f"{name}\t{groups[name]}" for name in sorted(groups))
    return "\n".join(lines) + "\n", groups


def source_hashes():
    paths = git("ls-tree", "-r", "--name-only", W2_COMMIT, "src", "include").splitlines()
    paths = sorted(
        path
        for path in paths
        if Path(path).suffix in {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}
    )
    if not paths:
        raise SystemExit("no committed W2 production source files found")
    return "".join(f"{sha256(blob(path))}  {path}\n" for path in paths), len(paths)


def resource_baseline():
    path = f"{W2}/resource_summary.csv"
    data = blob(path)
    rows = csv_rows(data)
    expected = [
        ("GATE4_0_W1", "boom_core_top", "product_baseline", "5.898", "51558", "12802", "12", "3"),
        ("GATE4_0_W2", "synth_issue_top", "diagnostic", "4.570", "15079", "4608", "0", "0"),
        ("GATE4_0_W2", "synth_core_step_top", "diagnostic", "5.898", "54667", "15123", "12", "3"),
        ("GATE4_0_W2", "boom_core_top", "product_checkpoint", "5.898", "61760", "15213", "12", "3"),
    ]
    fields = ("variant", "top", "scope", "period_ns", "lut", "ff", "bram_18k", "dsp")
    actual = [tuple(row[field] for field in fields) for row in rows]
    if actual != expected or any(row["core_cycle_pipeline"] != "no" for row in rows):
        raise SystemExit("committed W2 resource summary does not match the qualified baseline")
    output = io.StringIO(newline="")
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(list(rows[0]) + ["evidence_path", "evidence_sha256", "commit"])
    for row in rows:
        writer.writerow(list(row.values()) + [path, sha256(data), W2_COMMIT])
    return output.getvalue(), rows


def trace_manifest():
    trace_diff_path = f"{W2}/regression/trace_diff.md"
    trace_diff = blob(trace_diff_path)
    if b"Result: 10/10 byte-identical" not in trace_diff:
        raise SystemExit("committed W2 trace comparison is not 10/10 byte-identical")
    compared = {
        (program, producer)
        for program, producer in re.findall(
            r"\| ([a-z_]+) \| (hls_(?:cpp|csim)) \| PASS \|",
            trace_diff.decode("utf-8"),
        )
    }

    rows = []
    hls_prefix = f"{W2}/regression/hls_traces/"
    hls_paths = sorted(
        path
        for path in git("ls-tree", "-r", "--name-only", W2_COMMIT, hls_prefix).splitlines()
        if path.endswith(".jsonl")
    )
    for path in hls_paths:
        match = re.fullmatch(r"(.+)_hls_(cpp|csim)_full\.jsonl", Path(path).name)
        if not match:
            raise SystemExit(f"unexpected committed W2 trace name: {path}")
        program, kind = match.groups()
        producer = f"hls_{kind}"
        data = blob(path)
        rows.append([
            "GATE4_0_W2", producer, "", program, "PASS",
            "PASS_BYTE_IDENTICAL" if (program, producer) in compared else "NOT_IN_FROZEN_10_TRACE_SET",
            path, sha256(data), len(data), len(data.splitlines()), W2_COMMIT,
        ])

    matrix_path = f"{W2}/rtl_run_status.csv"
    matrix_data = blob(matrix_path)
    matrix = csv_rows(matrix_data)
    if len(matrix) != 49 or any(row["run_status"] != "XSIM_PASS" for row in matrix):
        raise SystemExit("committed W2 generated RTL matrix is not 49/49 PASS")
    for row in matrix:
        data = blob(row["trace"])
        rows.append([
            "GATE4_0_W2", "rtl_xsim", row["test"], row["program"], "PASS",
            "ARCHITECTURAL_PASS", row["trace"], sha256(data), len(data),
            len(data.splitlines()), W2_COMMIT,
        ])
    if len(hls_paths) != 14 or len(compared) != 10 or len(rows) != 63:
        raise SystemExit("unexpected committed W2 trace inventory")

    output = io.StringIO(newline="")
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow([
        "gate", "producer", "test", "program", "outcome", "comparison", "path",
        "sha256", "bytes", "records", "commit",
    ])
    writer.writerows(rows)
    return output.getvalue(), sha256(trace_diff), sha256(matrix_data)


def grant_accounting():
    directed_path = f"{W2}/regression/logs/w2_dual_grant_tests.log"
    random_path = f"{W2}/regression/logs/w2_dual_grant_random_tests.log"
    directed = blob(directed_path)
    random = blob(random_path)
    if b"=== 28 passed, 0 failed, 28 total ===" not in directed:
        raise SystemExit("committed W2 directed grant evidence is not 28/28 PASS")
    match = re.search(
        rb"seeds=(\d+) cycles/seed=(\d+) total_cycles=(\d+).*"
        rb"dual_grants=(\d+) accepts=(\d+) retained=(\d+) dropped=(\d+)",
        random,
        re.DOTALL,
    )
    if not match or tuple(map(int, match.groups())) != (64, 32, 2048, 63, 382, 424, 0):
        raise SystemExit("committed W2 random grant accounting does not match the qualified campaign")
    metrics = [
        ("directed_passed", 28, "assertions", directed_path, directed),
        ("directed_failed", 0, "assertions", directed_path, directed),
        ("random_seeds", 64, "seeds", random_path, random),
        ("cycles_per_seed", 32, "cycles", random_path, random),
        ("random_cycles", 2048, "cycles", random_path, random),
        ("dual_grants", 63, "cycles", random_path, random),
        ("accepted_grants", 382, "grants", random_path, random),
        ("retained_grants", 424, "grants", random_path, random),
        ("dropped_grants", 0, "grants", random_path, random),
    ]
    output = io.StringIO(newline="")
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(["metric", "value", "unit", "evidence_path", "evidence_sha256", "commit"])
    for metric, value, unit, path, data in metrics:
        writer.writerow([metric, value, unit, path, sha256(data), W2_COMMIT])
    return output.getvalue(), sha256(directed), sha256(random)


def regression_report(directed_sha, random_sha, trace_sha, rtl_sha):
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
        "# Gate 4.0 W3 Regression Baseline", "",
        f"Immutable W2 commit: `{W2_COMMIT}`.", "",
        "No tests were rerun. Every outcome below was parsed from a committed W2 blob.", "",
        "| Suite | Outcome | Passed | Failed | Runs | Evidence SHA-256 |",
        "|---|---|---:|---:|---:|---|",
    ]
    total = 0
    for label, name, expected, runs in suites:
        path = f"{W2}/regression/logs/{name}"
        data = blob(path)
        summaries = re.findall(rb"=== (\d+) passed, (\d+) failed ===", data)
        passed = sum(int(item[0]) for item in summaries)
        failed = sum(int(item[1]) for item in summaries)
        if passed != expected or failed or len(summaries) != runs:
            raise SystemExit(f"unexpected committed W2 test outcome in {path}")
        total += passed
        lines.append(f"| {label} | PASS | {passed} | {failed} | {runs} | `{sha256(data)}` |")

    lane_path = f"{W2}/regression/logs/gate4_w1_lane_interface_tests.log"
    lane_data = blob(lane_path)
    if b"Gate 4.0 W1 lane interface tests: PASS" not in lane_data:
        raise SystemExit("committed W1 lane-interface evidence is not PASS")
    lines.append(f"| W1 lane interface | PASS | 1 | 0 | 1 | `{sha256(lane_data)}` |")
    lines.append(f"| W2 dual-grant directed | PASS | 28 | 0 | 1 | `{directed_sha}` |")
    total += 29
    if total != 177:
        raise SystemExit(f"unexpected W2 assertion total: {total}")

    arch_path = f"{W2}/regression/full_program_architectural_diff.csv"
    arch_data = blob(arch_path)
    arch = csv_rows(arch_data)
    if len(arch) != 10 or any(row["status"] != "PASS" for row in arch):
        raise SystemExit("committed W2 architectural comparison is not 10/10 PASS")
    issue_path = f"{W2}/issue_rtl/results.csv"
    issue_data = blob(issue_path)
    issue = csv_rows(issue_data)
    if len(issue) != 5 or any(row["status"] != "PASS" for row in issue):
        raise SystemExit("committed W2 issue RTL result is not 5/5 PASS")

    lines.extend([
        "", "## Campaign And Integration Evidence", "",
        f"- Recorded source assertions: **177 passed, 0 failed**; W2 directed selection is **28/28 PASS**.",
        f"- W2 random differential: **64 seeds**, **2048 cycles**, **63 dual grants**, **382 accepted**, **424 retained**, **0 dropped**; evidence SHA-256 `{random_sha}`.",
        f"- Frozen HLS C++/csim trace comparison: **10/10 byte-identical**; evidence SHA-256 `{trace_sha}`.",
        f"- Full-program architectural comparison: **10/10 PASS**; evidence SHA-256 `{sha256(arch_data)}`.",
        f"- Dedicated synthesized issue-selection RTL: **5/5 PASS**; evidence SHA-256 `{sha256(issue_data)}`.",
        f"- Full generated-core XSim matrix: **49/49 PASS**; evidence SHA-256 `{rtl_sha}`.",
        "- W2 synthesis resources are frozen in `w2_resource_baseline.csv`.",
        "", "## Qualification", "",
        "The official Chipyard/FESVR/DRAMSim path remained unavailable. This freezes verification of the supported HLS subset and generated RTL, not full BOOM equivalence.",
        "",
    ])
    return "\n".join(lines)


def manifest(head, groups, source_count, resources):
    dirty = [
        f"- {name}: {count} porcelain status entr{'y' if count == 1 else 'ies'}"
        for name, count in sorted(groups.items())
    ] or ["- clean (excluding this generator and its seven W3 outputs)"]
    product = next(row for row in resources if row["scope"] == "product_checkpoint")
    return "\n".join([
        "# Gate 4.0 W3 Baseline Freeze", "",
        f"- Immutable W2 commit: `{W2_COMMIT}`",
        f"- Current commit at freeze: `{head}`",
        "- Evidence source: committed blobs from the immutable W2 commit, never dirty worktree copies",
        "", "## Frozen Baseline", "",
        f"- Production C/C++ source identities: {source_count} committed files in `source_hashes_before.txt`.",
        f"- W2 recorded assertions: 177 passed, 0 failed; W2 directed selection: 28/28 PASS.",
        "- W2 random differential: 64 seeds, 2048 cycles, 63 dual grants, 382 accepted grants, 424 retained grants, and 0 dropped grants.",
        "- Dedicated issue RTL: 5/5 PASS; full generated-core RTL: 49/49 PASS.",
        "- Frozen HLS C++/csim trace comparison: 10/10 byte-identical; architectural comparison: 10/10 PASS.",
        f"- W2 product checkpoint: {product['period_ns']} ns, {product['lut']} LUT, {product['ff']} FF, {product['bram_18k']} BRAM_18K, {product['dsp']} DSP; `CORE_CYCLE` unpipelined.",
        "- All W1 baseline and W2 diagnostic/product resource rows are frozen in `w2_resource_baseline.csv`.",
        "", "## Pre-existing Dirty Worktree Groups", "", *dirty, "",
        "`git_status_before.txt` records aggregate porcelain-status counts only. Backup paths and contents are intentionally omitted, and no pre-existing dirty path was read as baseline evidence or modified.",
        "", "## Artifacts", "",
        "- `baseline_manifest.md`: immutable commit, qualification summary, and grouped dirty state.",
        "- `git_status_before.txt`: aggregate-only pre-existing dirty status.",
        "- `source_hashes_before.txt`: SHA-256 identities of committed production sources.",
        "- `regression_before.md`: validated committed test, trace, architecture, and RTL outcomes.",
        "- `w2_resource_baseline.csv`: W2 resource rows and committed evidence identity.",
        "- `w2_trace_manifest.csv`: immutable identities for 14 HLS and 49 generated-RTL traces.",
        "- `w2_grant_accounting.csv`: directed and random grant metrics with evidence identities.",
        "", "## Reproduction", "",
        "Run `python3 scripts/gate4_0/freeze_w3_baseline.py` from anywhere in this repository. The script writes only the seven W3 report files listed above.",
        "", "## Limitations", "",
        "- This is an immutable evidence freeze, not a rerun of tests, csim, synthesis, or RTL simulation.",
        "- W2 `load_store` and `tohost` C++/csim traces are inventoried but were not members of the committed 10-trace byte-comparison set.",
        "- No strict BOOM cycle-equivalence or official full-system equivalence claim is made.",
        "",
    ])


def main():
    os.chdir(ROOT)
    resolved = git("rev-parse", f"{W2_COMMIT}^{{commit}}").strip()
    if resolved != W2_COMMIT:
        raise SystemExit(f"W2 commit resolved unexpectedly: {resolved}")
    head = git("rev-parse", "HEAD").strip()
    status, groups = grouped_status()
    sources, source_count = source_hashes()
    resources_csv, resources = resource_baseline()
    traces_csv, trace_sha, rtl_sha = trace_manifest()
    grants_csv, directed_sha, random_sha = grant_accounting()
    regression = regression_report(directed_sha, random_sha, trace_sha, rtl_sha)
    baseline = manifest(head, groups, source_count, resources)
    outputs = {
        "baseline_manifest.md": baseline,
        "git_status_before.txt": status,
        "source_hashes_before.txt": sources,
        "regression_before.md": regression,
        "w2_resource_baseline.csv": resources_csv,
        "w2_trace_manifest.csv": traces_csv,
        "w2_grant_accounting.csv": grants_csv,
    }
    if set(outputs) != OUTPUT_NAMES:
        raise SystemExit("internal output allowlist mismatch")
    for name, data in outputs.items():
        write_atomic(name, data)


if __name__ == "__main__":
    main()
