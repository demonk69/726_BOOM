#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR=${BOOM_W4E_BUILD_DIR:-"$ROOT/build/gate4_0/w4e_regression"}
REPORT_DIR=${BOOM_W4E_REPORT_DIR:-"$ROOT/reports/gate4_0/w4"}
RUN_DIR="$REPORT_DIR/regression/w4e"
LOG_DIR="$RUN_DIR/logs"
CXX_BIN=${CXX:-g++}
mkdir -p "$BUILD_DIR" "$LOG_DIR"

# This is intentionally software-only. Do not add an RTL or csynth launcher here.
BOOM_REGRESSION_BUILD_DIR="$BUILD_DIR/product_full" \
BOOM_REGRESSION_REPORT_DIR="$RUN_DIR/product_full" \
  "$ROOT/scripts/gate4_0/run_w3_regressions.sh"

COMMON_SRCS=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/fetch_packet.cpp"
  "$ROOT/src/fetch_buffer.cpp" "$ROOT/src/predecode.cpp" "$ROOT/src/predictor.cpp"
  "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp"
  "$ROOT/src/reset.cpp"
)

compile_run() {
  local name=$1 source=$2
  shift 2
  "$CXX_BIN" -std=c++11 -I"$ROOT/include" "$@" "${COMMON_SRCS[@]}" "$source" \
    -o "$BUILD_DIR/$name" > "$LOG_DIR/${name}_compile.log" 2>&1
  "$BUILD_DIR/$name" > "$LOG_DIR/$name.log" 2>&1
}

# Cumulative approved W4A/W4B/W4C/W4D directed software suites.
compile_run w4a_completion_interface_tests "$ROOT/tb/differential/w4a_completion_interface_tests.cpp" \
  -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC
compile_run w4b_multi_rob_complete_tests "$ROOT/tb/differential/w4b_multi_rob_complete_tests.cpp"
compile_run w4_multi_wakeup_tests "$ROOT/tb/differential/w4_multi_wakeup_tests.cpp"
compile_run w4_bypass_tests "$ROOT/tb/differential/w4_bypass_tests.cpp"
compile_run w4_multi_writeback_tests "$ROOT/tb/differential/w4_multi_writeback_tests.cpp"
compile_run w4_multi_completion_tests "$ROOT/tb/differential/w4_multi_completion_tests.cpp"
compile_run w4d_rtl_oracle_tests "$ROOT/tb/differential/w4d_rtl_oracle_tests.cpp" \
  "$ROOT/src/synth_module_tops.cpp"
compile_run w4_completion_random_tests "$ROOT/tb/differential/w4_completion_random_tests.cpp"

grep -q 'Software suites:.*400 passed, 0 failed' "$RUN_DIR/product_full/regression_after.md"
grep -q 'W4A completion interfaces: 19 passed, 0 failed' "$LOG_DIR/w4a_completion_interface_tests.log"
grep -q 'W4B multi ROB complete: 17 passed, 0 failed' "$LOG_DIR/w4b_multi_rob_complete_tests.log"
grep -q 'W4C multi wakeup: 14 passed, 0 failed' "$LOG_DIR/w4_multi_wakeup_tests.log"
grep -q 'W4D bypass: 14 passed, 0 failed' "$LOG_DIR/w4_bypass_tests.log"
grep -q 'W4D multi writeback: 13 passed, 0 failed' "$LOG_DIR/w4_multi_writeback_tests.log"
grep -q 'W4D multi completion: 7 passed, 0 failed' "$LOG_DIR/w4_multi_completion_tests.log"
grep -q 'W4D RTL oracle preparation: 11 passed, 0 failed' "$LOG_DIR/w4d_rtl_oracle_tests.log"
grep -q 'W4E independent persistent completion random differential: PASS' "$LOG_DIR/w4_completion_random_tests.log"

python3 - "$ROOT" "$RUN_DIR" "$REPORT_DIR" <<'PY'
import csv
import hashlib
import re
import sys
from pathlib import Path

root, run, report = map(Path, sys.argv[1:])
logs = run / "logs"

def metrics(path):
    text = path.read_text(errors="replace")
    pairs = re.findall(r"^METRIC,([^,]+),(\d+)$", text, re.M)
    if len(pairs) != len({name for name, _ in pairs}):
        raise SystemExit(f"duplicate metrics in {path}")
    return {name: int(value) for name, value in pairs}

w4 = metrics(logs / "w4_completion_random_tests.log")
required_exact = {
    "random_seeds": 128, "seed_pass_records": 128, "cycles_per_seed": 128,
    "production_order_collision_probes": 128,
    "total_random_cycles": 16384, "eligible_write_arbitration_wait_bound_cycles": 1,
    "peak_completion_sources": 3,
    "peak_prf_writes": 2, "peak_wakeups": 3, "peak_bypass": 3,
    "source_events_pending_final": 0, "tokens_pending_final": 0,
    "accepted_pending_final": 0, "dropped_tokens": 0, "duplicate_tokens": 0,
    "stale_side_effects": 0, "terminal_duplicate_classifications": 0,
    "terminal_missing_records": 0, "unexplained_tokens": 0,
}
for name, expected in required_exact.items():
    if w4.get(name) != expected:
        raise SystemExit(f"W4E guard failed: {name}={w4.get(name)} expected {expected}")
positive = (
    "bounded_drain_cycles", "tokens_offered", "tokens_committed", "tokens_killed",
    "tokens_faulted", "accepted_tokens", "accepted_terminal_committed",
    "accepted_then_killed", "accepted_terminal_faulted", "source_events_offered",
    "source_events_accepted", "source_events_rejected", "source_events_killed",
    "source_events_faulted", "int_completion_events", "mem_completion_events",
    "mem_agu_events", "store_events", "load_response_events", "stale_completion_events",
    "stale_response_events", "delayed_response_cycles", "post_reset_stale_responses",
    "branch_resolves", "branch_mispredicts", "precise_exception_events",
    "validation_faults", "rob_index_wraps", "rob_index_reuses", "allocation_id_wraps",
    "resets", "resets_with_inflight", "reset_killed_tokens",
    "branch_killed_tokens", "fence_blocked_cycles", "prf_writes", "rob_completes",
    "commits", "wakeup_publications", "bypass_publications",
    "repeated_wakeup_publications", "same_pdst_same_value",
    "same_pdst_different_value", "source_age_permutations", "sustained_arrival_cycles",
    "pending_pressure_cycles", "trace_backpressure_cycles", "dmem_backpressure_cycles",
    "iq_prs1_checks", "iq_prs2_checks", "iq_prs3_checks", "prf_value_checks",
    "prf_busy_checks", "exact_writeback_checks", "exact_wakeup_checks",
    "exact_rob_complete_checks", "offer_to_accept_latency_samples",
    "rob_complete_to_commit_latency_samples", "stale_snapshot_checks",
    "full_branch_update_checks", "fault_metadata_checks", "commit_payload_checks",
    "lsu_ownership_checks", "csr_counter_checks", "full_load_request_checks",
)
for name in positive:
    if w4.get(name, 0) <= 0:
        raise SystemExit(f"W4E coverage guard failed: {name}={w4.get(name)}")
minimums = {
    "resets_with_inflight": 128, "post_reset_stale_responses": 128,
    "branch_killed_tokens": 128, "fence_blocked_cycles": 128,
    "sustained_arrival_cycles": 128, "source_age_permutations": 128,
    "same_pdst_same_value": 128, "same_pdst_different_value": 128,
    "validation_faults": 128, "pending_pressure_cycles": 1000,
    "exact_writeback_checks": 1000, "exact_wakeup_checks": 1000,
    "exact_rob_complete_checks": 1000,
}
for name, minimum in minimums.items():
    if w4.get(name, 0) < minimum:
        raise SystemExit(f"W4E threshold failed: {name}={w4.get(name)} minimum {minimum}")
for mask in range(8):
    name = f"source_combination_mask_{mask}"
    if w4.get(name, 0) <= 0:
        raise SystemExit(f"W4E missing source combination: {name}")
    positive += (name,)
equations = (
    ("source_conservation_lhs", "source_conservation_rhs"),
    ("token_conservation_lhs", "token_conservation_rhs"),
    ("accepted_conservation_lhs", "accepted_conservation_rhs"),
    ("wakeup_publications", "bypass_publications"),
    ("exact_writeback_checks", "prf_writes"),
    ("exact_wakeup_checks", "wakeup_publications"),
    ("exact_rob_complete_checks", "rob_completes"),
    ("resets", "resets_with_inflight"),
    ("stale_snapshot_checks", "source_events_rejected"),
)
for lhs, rhs in equations:
    if w4.get(lhs) != w4.get(rhs):
        raise SystemExit(f"W4E equation failed: {lhs}={w4.get(lhs)} {rhs}={w4.get(rhs)}")
if w4["max_eligible_write_arbitration_wait_cycles"] > w4["eligible_write_arbitration_wait_bound_cycles"]:
    raise SystemExit("W4E eligible write-arbitration wait bound exceeded")
if w4["source_events_offered"] != (w4["source_events_accepted"] +
        w4["source_events_rejected"] + w4["source_events_killed"] +
        w4["source_events_faulted"] + w4["source_events_pending_final"]):
    raise SystemExit("W4E expanded source conservation failed")
if w4["tokens_offered"] != (w4["tokens_committed"] + w4["tokens_killed"] +
                             w4["tokens_faulted"] + w4["tokens_pending_final"]):
    raise SystemExit("W4E expanded token conservation failed")
if w4["accepted_tokens"] != (w4["accepted_terminal_committed"] +
        w4["accepted_then_killed"] + w4["accepted_terminal_faulted"] +
        w4["accepted_pending_final"]):
    raise SystemExit("W4E expanded accepted-token conservation failed")

text = (logs / "w4_completion_random_tests.log").read_text(errors="replace")
seed_rows = re.findall(r"^SEED_PASS,index=(\d+),seed=(0x[0-9a-f]+),random_cycles=(\d+),"
                       r"drain_cycles=(\d+),resets=(\d+),tokens=(\d+),events=(\d+)$", text, re.M)
if len(seed_rows) != 128 or [int(row[0]) for row in seed_rows] != list(range(128)):
    raise SystemExit("W4E does not contain 128 unique per-seed PASS records")
if any(int(row[2]) != 128 or int(row[4]) <= 0 for row in seed_rows):
    raise SystemExit("W4E per-seed cycle/reset accounting failed")
expected_seeds = [0x6a09e667 ^ ((0x9e3779b9 * (index + 1)) & 0xffffffff)
                  for index in range(128)]
observed_seeds = [int(row[1], 16) for row in seed_rows]
if len(set(observed_seeds)) != 128 or observed_seeds != expected_seeds:
    raise SystemExit("W4E seed values are not unique or do not match the deterministic sequence")

requirements = {name: ">0" for name in positive}
requirements.update({name: f">={value}" for name, value in minimums.items()})
requirements.update({name: f"={value}" for name, value in required_exact.items()})
requirements["max_eligible_write_arbitration_wait_cycles"] = \
    "<=eligible_write_arbitration_wait_bound_cycles"
for lhs, rhs in equations:
    requirements[lhs] = f"={rhs}"
with (report / "concurrency_metrics.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("metric", "observed", "requirement", "status"))
    for name in sorted(w4):
        requirement = requirements.get(name, "observed")
        writer.writerow((name, w4[name], requirement, "PASS"))

latencies = (
    ("source_offer", "source_accept", "offer_to_accept_latency_samples",
     "offer_to_accept_latency_sum", "offer_to_accept_latency_max"),
    ("rob_complete", "commit", "rob_complete_to_commit_latency_samples",
     "rob_complete_to_commit_latency_sum", "rob_complete_to_commit_latency_max"),
)
with (run / "latency_metrics.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("start_stage", "end_stage", "samples", "cycle_sum", "average_cycles", "max_cycles"))
    for start, end, samples_name, sum_name, max_name in latencies:
        samples, total = w4[samples_name], w4[sum_name]
        writer.writerow((start, end, samples, total, f"{total / samples:.6f}", w4[max_name]))

suite_specs = (
    ("W4A directed", "w4a_completion_interface_tests.log", 19),
    ("W4B directed", "w4b_multi_rob_complete_tests.log", 17),
    ("W4C wakeup", "w4_multi_wakeup_tests.log", 14),
    ("W4D bypass", "w4_bypass_tests.log", 14),
    ("W4D writeback", "w4_multi_writeback_tests.log", 13),
    ("W4D completion", "w4_multi_completion_tests.log", 7),
    ("W4D oracle preparation", "w4d_rtl_oracle_tests.log", 11),
)
suite_rows = []
for suite, filename, expected_passed in suite_specs:
    path = logs / filename
    outcomes = re.findall(r"(\d+) passed, (\d+) failed", path.read_text(errors="replace"))
    if len(outcomes) != 1:
        raise SystemExit(f"ambiguous parsed suite result: {suite} outcomes={outcomes}")
    passed, failed = map(int, outcomes[0])
    if passed != expected_passed or failed:
        raise SystemExit(f"unexpected parsed suite result: {suite} {passed}/{failed}")
    suite_rows.append((suite, path, passed, failed))
suite_rows.append(("W4E random seeds", logs / "w4_completion_random_tests.log",
                   len(seed_rows), 0))
directed_passed = sum(row[2] for row in suite_rows[:-1])
with (run / "suite_results.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("suite", "status", "passed", "failed", "log", "sha256"))
    for suite, path, passed, failed in suite_rows:
        writer.writerow((suite, "PASS" if failed == 0 else "FAIL", passed, failed,
                         f"logs/{path.name}",
                         hashlib.sha256(path.read_bytes()).hexdigest()))

full_program = list(csv.DictReader((run / "product_full/full_program_architectural_diff.csv").open()))
trace = list(csv.DictReader((run / "product_full/trace_comparison.csv").open()))
normalized = list(csv.DictReader((run / "product_full/normalized_trace_diff.csv").open()))
if sum(row.get("status") == "PASS" for row in full_program) != 10:
    raise SystemExit("full-program architectural diff is not 10/10")
if sum(row.get("normalized_status") == "PASS" for row in trace) != len(trace):
    raise SystemExit("C++/csim trace comparison failed")
if sum(row.get("status") == "PASS" for row in normalized) != len(normalized):
    raise SystemExit("normalized event/cycle comparison failed")
partial = (run / "product_full/logs/partial_order.log").read_text(errors="replace")
if len(re.findall(r"^LEGAL_REORDER ", partial, re.M)) != 7:
    raise SystemExit("partial-order checks are not 7/7")

summary = [
    "# Gate 4.0 W4E Software Regression After", "",
    "Result: **PASS** for W4E software/random integration on the approved W4D product.", "",
    f"- W4E independent persistent random: {w4['seed_pass_records']}/{w4['random_seeds']} fixed seeds, "
    f"{w4['total_random_cycles']}/{w4['total_random_cycles']} randomized cycles.",
    f"- Eligible write-port arbitration wait only: observed "
    f"{w4['max_eligible_write_arbitration_wait_cycles']} cycles, bound "
    f"{w4['eligible_write_arbitration_wait_bound_cycles']} = "
    f"ceil(3 fixed source holds / 2 writers) - 1. This is not a general completion-liveness claim; "
    f"{w4['fence_blocked_cycles']} fence-blocked cycles are classified separately.",
    f"- Publication peaks: {w4['peak_completion_sources']} sources, {w4['peak_prf_writes']} PRF writes, "
    f"{w4['peak_wakeups']} wakeups, {w4['peak_bypass']} bypasses.",
    f"- Token conservation: {w4['token_conservation_lhs']} offered = "
    f"{w4['tokens_committed']} committed + {w4['tokens_killed']} killed + "
    f"{w4['tokens_faulted']} faulted; source-event conservation: "
    f"{w4['source_conservation_lhs']} = {w4['source_conservation_rhs']}.",
    f"- Accepted-token conservation: {w4['accepted_conservation_lhs']} = "
    f"{w4['accepted_terminal_committed']} committed + {w4['accepted_then_killed']} killed + "
    f"{w4['accepted_terminal_faulted']} faulted + {w4['accepted_pending_final']} pending.",
    f"- Integrity: {w4['dropped_tokens']} dropped, {w4['duplicate_tokens']} duplicated, "
    f"{w4['stale_side_effects']} stale side effects, {w4['unexplained_tokens']} unexplained.",
    f"- Reset/branch recovery: {w4['resets_with_inflight']} in-flight resets, "
    f"{w4['post_reset_stale_responses']} delayed stale post-reset responses, "
    f"{w4['branch_killed_tokens']} branch-killed tokens.",
    f"- Full load `DmemRequest` payload checks: {w4['full_load_request_checks']}.",
    f"- Cumulative W4A/W4B/W4C/W4D directed suites: {directed_passed}/{directed_passed} PASS, "
    "derived from parsed suite records.",
    "- W3 canonical software preservation: 400/400 PASS; includes W2/W3 random and reset suites.",
    f"- C++/csim normalized traces: {len(trace)}/{len(trace)} PASS; normalized architecture/event/cycle "
    f"checks: {len(normalized)}/{len(normalized)} PASS.",
    "- Full-program architectural diff: 10/10 PASS; partial order: 7/7 PASS.",
    "- Merged generation, merged compile, synth-top compile, and core-top compile: 4/4 PASS.", "",
    "Observed concurrency and conservation fields are in `concurrency_metrics.csv`. Only genuinely "
    "measured stage latencies are reported, in `regression/w4e/latency_metrics.csv`; unlike W3/W4E "
    "event subtraction is intentionally absent.", "",
    "This is not a final W4 claim. Final RTL and csynth were intentionally not run.",
]
(report / "regression_after.md").write_text("\n".join(summary) + "\n", encoding="utf-8")
PY

printf '%s\n' 'Gate 4.0 W4E software regressions complete; RTL and csynth intentionally not run.'
