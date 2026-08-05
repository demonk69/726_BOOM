#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR=${BOOM_REGRESSION_BUILD_DIR:-"$ROOT/build/gate4_0/w3_regression"}
REPORT_DIR=${BOOM_REGRESSION_REPORT_DIR:-"$ROOT/reports/gate4_0/w3/regression"}
LOG_DIR="$REPORT_DIR/logs"
TRACE_DIR="$REPORT_DIR/hls_traces"
NORMALIZED_DIR="$REPORT_DIR/normalized"
CXX_BIN=${CXX:-g++}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}

mkdir -p "$BUILD_DIR" "$LOG_DIR" "$TRACE_DIR" "$NORMALIZED_DIR"

COMMON_SRCS=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/completion.cpp"
  "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
)

compile_test() {
  local name=$1
  local source=$2
  shift 2
  "$CXX_BIN" -std=c++11 -I"$ROOT/include" "$@" "${COMMON_SRCS[@]}" "$source" \
    -o "$BUILD_DIR/$name" > "$LOG_DIR/${name}_compile.log" 2>&1
}

run_test() {
  local name=$1
  "$BUILD_DIR/$name" > "$LOG_DIR/$name.log" 2>&1
}

# Generation and all public HLS top translation units must remain compilable.
"$ROOT/scripts/generate_merged.sh" > "$LOG_DIR/generate_merged.log" 2>&1
"$CXX_BIN" -std=c++11 -I"$ROOT/include" -c "$ROOT/src/boom_core_merged.cpp" \
  -o "$BUILD_DIR/boom_core_merged.o" > "$LOG_DIR/merged_compile.log" 2>&1
"$CXX_BIN" -std=c++11 -I"$ROOT/include" -c "$ROOT/src/synth_module_tops.cpp" \
  -o "$BUILD_DIR/synth_module_tops.o" > "$LOG_DIR/synth_tops_compile.log" 2>&1
"$CXX_BIN" -std=c++11 -I"$ROOT/include" -c "$ROOT/src/boom_core_top.cpp" \
  -o "$BUILD_DIR/boom_core_top.o" > "$LOG_DIR/core_top_compile.log" 2>&1

for test in dispatch_retry_tests w3_dual_execute_tests w3_completion_buffer_tests \
  w3_dual_execute_random_tests directed_tests gate1_regression_tests \
  lsu_minimal_tests branch_snapshot_tests branch_snapshot_random_tests \
  iq_compaction_tests gate4_w1_lane_interface_tests w2_dual_grant_tests \
  w2_dual_grant_random_tests; do
  compile_test "$test" "$ROOT/tb/differential/$test.cpp"
done
compile_test reset_architecture_tests "$ROOT/tb/differential/reset_architecture_tests.cpp"
compile_test reset_architecture_pipelined_tests \
  "$ROOT/tb/differential/reset_architecture_tests.cpp" \
  -DBOOM_HLS_GATE3_10_R1_RESET_ROB_PIPELINE

for test in dispatch_retry_tests w3_dual_execute_tests w3_completion_buffer_tests \
  w3_dual_execute_random_tests directed_tests gate1_regression_tests \
  lsu_minimal_tests branch_snapshot_tests iq_compaction_tests \
  gate4_w1_lane_interface_tests w2_dual_grant_tests \
  w2_dual_grant_random_tests reset_architecture_tests \
  reset_architecture_pipelined_tests; do
  run_test "$test"
done

: > "$LOG_DIR/branch_snapshot_random_tests.log"
for seed in default 0x13579bdf 0x2468ace0 0x10203040 0x55667788 0xdeadbeef \
  0x0badc0de 0xc001d00d 0x31415926 0x27182818 0xabcdef01 0x12345678 \
  0x87654321 0xfeedface 0x600dcafe 0x5eed1234 0x01020304 0x89abcdef \
  0xf00df00d 0x55aa55aa 0xaa55aa55; do
  "$BUILD_DIR/branch_snapshot_random_tests" "$seed" \
    >> "$LOG_DIR/branch_snapshot_random_tests.log" 2>&1
done

compile_test hls_prefix_trace_tb "$ROOT/tb/differential/hls_prefix_trace_tb.cpp"
HLS_PROJECT_ROOT="$ROOT" HLS_TRACE_OUT_DIR="$TRACE_DIR" HLS_TRACE_SOURCE=hls_cpp \
  HLS_TRACE_MODE=complete "$BUILD_DIR/hls_prefix_trace_tb" \
  > "$LOG_DIR/hls_cpp_trace.log" 2>&1

if command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  VITIS_HLS_BIN=$(command -v "$VITIS_HLS_BIN")
else
  VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
fi
if [[ ! -x "$VITIS_HLS_BIN" ]]; then
  printf 'Vitis HLS not found: %s\n' "$VITIS_HLS_BIN" >&2
  exit 127
fi
(
  cd "$BUILD_DIR"
  FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    HLS_PROJECT_ROOT="$ROOT" HLS_TRACE_OUT_DIR="$TRACE_DIR" \
    HLS_TRACE_SOURCE=hls_csim HLS_TRACE_MODE=complete \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_hls_prefix_trace_csim.tcl"
) > "$LOG_DIR/hls_csim_trace.log" 2>&1

python3 "$ROOT/scripts/run_full_program_arch_diff.py" --root "$ROOT" \
  --hls-source hls_cpp --hls-source hls_csim --hls-trace-dir "$TRACE_DIR" \
  --csv-output "$REPORT_DIR/full_program_architectural_diff.csv" \
  --md-output "$REPORT_DIR/full_program_architectural_diff.md" \
  > "$LOG_DIR/full_program_arch_diff.log" 2>&1

# Normalize the seven current trace pairs and check architecture, event order,
# cycle-normalized order, and partial order without relying on W2 artifacts.
PYTHONPATH="$ROOT/scripts" python3 - "$TRACE_DIR" "$NORMALIZED_DIR" \
  "$REPORT_DIR/normalized_trace_diff.csv" "$REPORT_DIR/trace_comparison.csv" <<'PY'
import csv
import hashlib
import sys
from pathlib import Path
from provisional_gate3_lib import compare_normalized, load_jsonl, normalize_hls, write_jsonl

trace_dir, norm_dir, diff_csv, pair_csv = map(Path, sys.argv[1:])
programs = ("branch_not_taken", "branch_taken", "independent_alu", "nested_branch",
            "raw_chain", "load_store", "tohost")
norm_dir.mkdir(parents=True, exist_ok=True)
pairs = []
diffs = []
failed = False
for program in programs:
    paths = {}
    hashes = {}
    for source in ("hls_cpp", "hls_csim"):
        path = trace_dir / f"{program}_{source}_full.jsonl"
        if not path.is_file():
            raise FileNotFoundError(path)
        paths[source] = path
        hashes[source] = hashlib.sha256(path.read_bytes()).hexdigest()
        normalized = normalize_hls(load_jsonl(path), program, source)
        out = norm_dir / f"{program}_{source}.jsonl"
        write_jsonl(out, normalized)
    raw_identical = paths["hls_cpp"].read_bytes() == paths["hls_csim"].read_bytes()
    ref = load_jsonl(norm_dir / f"{program}_hls_cpp.jsonl")
    dut = load_jsonl(norm_dir / f"{program}_hls_csim.jsonl")
    pair_ok = True
    for kind in ("arch", "event", "cycle"):
        result = compare_normalized(ref, dut, kind)
        diffs.append((program, kind, result["status"], result["ref_events"],
                      result["dut_events"], result["compared_events"],
                      result["first_mismatch_index"]))
        pair_ok &= result["status"] == "PASS"
        failed |= result["status"] != "PASS"
    pairs.append((program, "PASS" if pair_ok else "FAIL", str(raw_identical).upper(),
                  hashes["hls_cpp"], hashes["hls_csim"],
                  paths["hls_cpp"].stat().st_size, paths["hls_csim"].stat().st_size))
with pair_csv.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "normalized_status", "raw_byte_identical", "hls_cpp_sha256",
                     "hls_csim_sha256", "hls_cpp_bytes", "hls_csim_bytes"))
    writer.writerows(pairs)
with diff_csv.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "kind", "status", "ref_events", "dut_events",
                     "compared_events", "first_mismatch_index"))
    writer.writerows(diffs)
raise SystemExit(1 if failed else 0)
PY

"$CXX_BIN" -std=c++11 -I"$ROOT/tb/differential" \
  "$ROOT/tb/differential/partial_order_matcher.cpp" \
  -o "$BUILD_DIR/partial_order_matcher" > "$LOG_DIR/partial_order_compile.log" 2>&1
: > "$LOG_DIR/partial_order.log"
for program in branch_not_taken branch_taken independent_alu nested_branch raw_chain load_store tohost; do
  "$BUILD_DIR/partial_order_matcher" "$NORMALIZED_DIR/${program}_hls_cpp.jsonl" \
    "$NORMALIZED_DIR/${program}_hls_csim.jsonl" >> "$LOG_DIR/partial_order.log" 2>&1
done

# Produce canonical counts and hashes only after every required command succeeds.
python3 - "$ROOT" "$REPORT_DIR" <<'PY'
import csv
import hashlib
import re
import sys
from pathlib import Path

root, report = map(Path, sys.argv[1:])
logs = report / "logs"
specs = [
    ("W3 dispatch retry", "dispatch_retry_tests.log", 12, 1),
    ("W3 dual execute", "w3_dual_execute_tests.log", 15, 1),
    ("W3 completion buffer", "w3_completion_buffer_tests.log", 18, 1),
    ("W2 dual-grant directed", "w2_dual_grant_tests.log", 28, 1),
    ("Directed architectural", "directed_tests.log", 25, 1),
    ("Gate 1", "gate1_regression_tests.log", 13, 1),
    ("LSU", "lsu_minimal_tests.log", 14, 1),
    ("Reset default", "reset_architecture_tests.log", 14, 1),
    ("Reset pipelined", "reset_architecture_pipelined_tests.log", 14, 1),
    ("Branch directed", "branch_snapshot_tests.log", 30, 1),
    ("IQ compaction", "iq_compaction_tests.log", 10, 1),
    ("W1 lane interface", "gate4_w1_lane_interface_tests.log", 1, 1),
]
rows = []
for suite, name, expected, runs in specs:
    path = logs / name
    text = path.read_text(errors="replace")
    failed = sum(int(x) for x in re.findall(r"(\d+) failed", text))
    if suite == "W1 lane interface":
        passed = 1 if "PASS" in text else 0
    else:
        matches = re.findall(r"(\d+) passed", text)
        passed = sum(map(int, matches))
    if passed != expected or failed:
        raise SystemExit(f"unexpected result for {suite}: passed={passed} failed={failed}")
    rows.append((suite, "PASS", passed, failed, runs,
                 f"logs/{name}", hashlib.sha256(path.read_bytes()).hexdigest()))

def exact_metric(text, name):
    values = re.findall(rf"^METRIC,{re.escape(name)},(\d+)$", text, re.M)
    if len(values) != 1:
        raise SystemExit(f"expected one {name} metric, found {len(values)}")
    return int(values[0])

random_specs = []
w3_path = logs / "w3_dual_execute_random_tests.log"
w3_text = w3_path.read_text(errors="replace")
w3_seeds = exact_metric(w3_text, "random_seeds")
w3_cycles = exact_metric(w3_text, "cycles_per_seed")
w3_total = exact_metric(w3_text, "total_random_cycles")
if "W3 persistent dual issue/execute/ROB/LSU random differential: PASS" not in w3_text:
    raise SystemExit("missing W3 persistent-random PASS marker")
if (w3_seeds, w3_cycles, w3_total) != (100, 64, 6400) or w3_total != w3_seeds * w3_cycles:
    raise SystemExit(f"unexpected W3 random dimensions: seeds={w3_seeds} cycles={w3_cycles} total={w3_total}")
random_specs.append(("W3 persistent random", w3_path, w3_seeds, w3_seeds))

w2_path = logs / "w2_dual_grant_random_tests.log"
w2_text = w2_path.read_text(errors="replace")
w2_dimensions = re.findall(r"seeds=(\d+) cycles/seed=(\d+) total_cycles=(\d+)", w2_text)
if len(w2_dimensions) != 1 or "W2 dual-grant random differential: PASS" not in w2_text:
    raise SystemExit("missing or ambiguous W2 random run accounting")
w2_seeds, w2_cycles, w2_total = map(int, w2_dimensions[0])
if (w2_seeds, w2_cycles, w2_total) != (64, 32, 2048) or w2_total != w2_seeds * w2_cycles:
    raise SystemExit(f"unexpected W2 random dimensions: seeds={w2_seeds} cycles={w2_cycles} total={w2_total}")
random_specs.append(("W2 dual-grant random", w2_path, w2_seeds, w2_seeds))

branch_path = logs / "branch_snapshot_random_tests.log"
branch_text = branch_path.read_text(errors="replace")
branch_seeds = re.findall(r"^Seed:\s+(\S+)\s*$", branch_text, re.M)
branch_passed = sum(map(int, re.findall(r"===\s+(\d+) passed,\s+0 failed\s+===", branch_text)))
branch_failed = sum(map(int, re.findall(r"(\d+) failed", branch_text)))
if len(branch_seeds) != 21 or len(set(branch_seeds)) != 21 or branch_passed != 42 or branch_failed:
    raise SystemExit(f"unexpected branch random accounting: seeds={len(branch_seeds)} unique={len(set(branch_seeds))} passed={branch_passed} failed={branch_failed}")
random_specs.append(("Branch random", branch_path, branch_passed, len(branch_seeds)))

for suite, path, passed, runs in random_specs:
    rows.append((suite, "PASS", passed, 0, runs,
                 f"logs/{path.name}", hashlib.sha256(path.read_bytes()).hexdigest()))
with (report / "suite_results.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("suite", "status", "passed", "failed", "runs", "log", "sha256"))
    writer.writerows(rows)

metrics = []
for campaign in ("w2_dual_grant_random_tests", "w3_dual_execute_random_tests"):
    text = (logs / f"{campaign}.log").read_text(errors="replace")
    for name, value in re.findall(r"METRIC,([^,\n]+),(\d+)", text):
        metrics.append((campaign, name, value))
w2_text = (logs / "w2_dual_grant_random_tests.log").read_text(errors="replace")
w2_patterns = {
    "random_seeds": r"seeds=(\d+)", "cycles_per_seed": r"cycles/seed=(\d+)",
    "total_random_cycles": r"total_cycles=(\d+)", "class_int": r"classes\(INT/MEM/UNSUP\)=(\d+)/",
    "class_mem": r"classes\(INT/MEM/UNSUP\)=\d+/(\d+)/", "class_unsupported": r"classes\(INT/MEM/UNSUP\)=\d+/\d+/(\d+)",
    "branch_resolves": r"branch\(resolve/mispredict/killed\)=(\d+)/", "branch_mispredicts": r"branch\(resolve/mispredict/killed\)=\d+/(\d+)/",
    "branch_kills": r"branch\(resolve/mispredict/killed\)=\d+/\d+/(\d+)", "dispatch_total": r"dispatch\(total/bypass/enqueue\)=(\d+)/",
    "dispatch_bypass": r"dispatch\(total/bypass/enqueue\)=\d+/(\d+)/", "dispatch_enqueue": r"dispatch\(total/bypass/enqueue\)=\d+/\d+/(\d+)",
    "port_ready_mask_00": r"port_ready_masks\(00/01/10/11\)=(\d+)/", "port_ready_mask_01": r"port_ready_masks\(00/01/10/11\)=\d+/(\d+)/",
    "port_ready_mask_10": r"port_ready_masks\(00/01/10/11\)=\d+/\d+/(\d+)/", "port_ready_mask_11": r"port_ready_masks\(00/01/10/11\)=\d+/\d+/\d+/(\d+)",
    "dual_grants": r"dual_grants=(\d+)", "accepted_grants": r"accepts=(\d+)",
    "retained_grants": r"retained=(\d+)", "dropped_grants": r"dropped=(\d+)",
}
for name, pattern in w2_patterns.items():
    match = re.search(pattern, w2_text)
    if not match:
        raise SystemExit(f"missing W2 metric: {name}")
    metrics.append(("w2_dual_grant_random_tests", name, match.group(1)))
metrics.extend([
    ("branch_snapshot_random_tests", "random_seed_runs", str(len(branch_seeds))),
    ("branch_snapshot_random_tests", "passed_checks", str(branch_passed)),
])
with (report / "random_metrics.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("campaign", "metric", "value"))
    writer.writerows(metrics)

source_paths = sorted(list((root / "include").glob("*.hpp")) +
                      [path for path in (root / "src").glob("*.cpp") if path.name != "boom_all.cpp"] +
                      list((root / "tb/differential").glob("*.cpp")) +
                      [root / "scripts/gate4_0/run_w3_regressions.sh"])
with (report / "source_hashes_after.txt").open("w", encoding="utf-8") as stream:
    for path in source_paths:
        stream.write(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(root)}\n")

artifacts = []
for path in sorted(report.rglob("*")):
    if path.is_file() and path.name not in ("artifact_hashes.csv", "regression_after.md"):
        artifacts.append((str(path.relative_to(report)), path.stat().st_size,
                          hashlib.sha256(path.read_bytes()).hexdigest()))
with (report / "artifact_hashes.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("path", "bytes", "sha256"))
    writer.writerows(artifacts)

with (report / "full_program_architectural_diff.csv").open(newline="", encoding="utf-8") as stream:
    arch = list(csv.DictReader(stream))
with (report / "trace_comparison.csv").open(newline="", encoding="utf-8") as stream:
    traces = list(csv.DictReader(stream))
with (report / "normalized_trace_diff.csv").open(newline="", encoding="utf-8") as stream:
    normalized = list(csv.DictReader(stream))
total_passed = sum(row[2] for row in rows)
lines = [
    "# Gate 4.0 W3 Regression After", "",
    f"Result: **PASS**. Software suites: **{len(rows)}/{len(rows)} PASS**, "
    f"**{total_passed} passed, 0 failed** across **{sum(row[4] for row in rows)} runs**.", "",
    f"- C++ vs HLS csim normalized trace comparison: {sum(r['normalized_status'] == 'PASS' for r in traces)}/{len(traces)} PASS.",
    f"- Normalized architecture/event/cycle checks: {sum(r['status'] == 'PASS' for r in normalized)}/{len(normalized)} PASS.",
    f"- Full-program architectural diff: {sum(r.get('status') == 'PASS' for r in arch)}/{len(arch)} PASS.",
    "- Partial-order checks: 7/7 PASS; event-order checks are included in the normalized CSV.",
    "- Merged generation, merged compile, synth-top compile, and core-top compile: 4/4 PASS.", "",
    "Exact per-suite counts and log hashes are in `suite_results.csv`; trace hashes are in "
    "`trace_comparison.csv`; campaign metrics are in `random_metrics.csv`; all canonical "
    "artifact hashes are in `artifact_hashes.csv`.",
    "", "Source scope is the modular `src/*.cpp` implementation, generated "
    "`src/boom_core_merged.cpp`, and `src/boom_core_top.cpp`. The unreferenced legacy "
    "`src/boom_all.cpp` snapshot is excluded from compilation, evidence, and acceptance.",
]
(report / "regression_after.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
PY

printf '%s\n' 'Gate 4.0 W3 canonical regressions complete.'
