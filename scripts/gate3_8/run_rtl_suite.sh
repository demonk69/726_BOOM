#!/usr/bin/env bash
set -uo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
RESULTS="$ROOT/reports/gate3_8/rtl_run_status.csv"
SUITE_LOG_DIR="$ROOT/reports/gate3_8/logs/suite"
mkdir -p "$SUITE_LOG_DIR"

CASES=(
  "R0_POWER_ON_RESET:independent_alu"
  "R1_RESET_FRONTEND_OUTSTANDING:independent_alu"
  "R2_RESET_ROB_NONEMPTY:raw_chain"
  "R3_RESET_IQ_NONEMPTY:raw_chain"
  "R4_RESET_LOAD_PENDING:load_store"
  "R5_RESET_STORE_PENDING:tohost"
  "R6_RESET_BRANCH_RECOVERY:branch_taken"
  "R7_RESET_TRACE_BACKPRESSURE:independent_alu"
  "B0_TRACE_READY_ALWAYS:independent_alu"
  "B1_TRACE_STALL_1:raw_chain"
  "B2_TRACE_STALL_BURST:branch_taken"
  "B3_TRACE_RANDOM_STALL:nested_branch"
  "B4_TRACE_STALL_AT_BRANCH_COMMIT:branch_taken"
  "B5_TRACE_STALL_AT_STORE_COMMIT:independent_alu"
  "I0_IMEM_READY_ALWAYS:independent_alu"
  "I1_IMEM_REQ_STALL:raw_chain"
  "I2_IMEM_RESPONSE_DELAY_1:independent_alu"
  "I3_IMEM_RESPONSE_DELAY_4:branch_not_taken"
  "I4_IMEM_RANDOM_DELAY:nested_branch"
  "I5_IMEM_STALE_RESPONSE_AFTER_REDIRECT:branch_taken"
  "I6_IMEM_RESPONSE_DURING_RESET:independent_alu"
  "D0_DMEM_READY_ALWAYS:load_store"
  "D1_STORE_REQ_STALL:independent_alu"
  "D2_LOAD_REQ_STALL:load_store"
  "D3_LOAD_RESPONSE_DELAY:load_store"
  "D4_LOAD_RANDOM_DELAY:load_store"
  "D5_STALE_LOAD_RESPONSE_AFTER_FLUSH:load_store"
  "D6_STORE_DURING_BRANCH_FLUSH:branch_taken"
  "D7_DMEM_RESPONSE_DURING_RESET:load_store"
  "D8_TOHOST_STORE_BACKPRESSURE:tohost"
  "P0_RESET_AND_BRANCH_MISPREDICT:branch_taken"
  "P1_RESET_AND_LOAD_RESPONSE:load_store"
  "P2_RESET_AND_STORE_ACCEPT:tohost"
  "P3_EXCEPTION_AND_BRANCH_MISPREDICT:branch_taken"
  "P4_BRANCH_MISPREDICT_AND_LOAD_RESPONSE:load_store"
  "P5_COMMIT_AND_TRACE_BACKPRESSURE:independent_alu"
  "P6_COMMIT_AND_STORE_BACKPRESSURE:tohost"
  "P7_REDIRECT_AND_IMEM_RESPONSE:branch_taken"
  "N0_NORMAL_INDEPENDENT_ALU:independent_alu"
  "N1_NORMAL_RAW_CHAIN:raw_chain"
  "N2_NORMAL_BRANCH_TAKEN:branch_taken"
  "N3_NORMAL_BRANCH_NOT_TAKEN:branch_not_taken"
  "N4_NORMAL_NESTED_BRANCH:nested_branch"
  "N5_NORMAL_LOAD_STORE:load_store"
  "N6_NORMAL_TOHOST:tohost"
)

printf '%s\n' 'test,program,run_status,runtime_seconds,log,trace' > "$RESULTS"
overall=0
for item in "${CASES[@]}"; do
  scenario=${item%%:*}
  program=${item#*:}
  start=$SECONDS
  wrapper_log="$SUITE_LOG_DIR/${program}_${scenario}.stdout.log"
  if "$ROOT/scripts/gate3_8/run_xsim.sh" "$scenario" "$program" > "$wrapper_log" 2>&1; then
    status=XSIM_PASS
  else
    status=XSIM_FAIL
    overall=1
  fi
  runtime=$((SECONDS - start))
  printf '%s,%s,%s,%s,%s,%s\n' "$scenario" "$program" "$status" "$runtime" \
    "reports/gate3_8/logs/${program}_${scenario}.log" \
    "reference/rtl_traces/${program}_${scenario}.jsonl" >> "$RESULTS"
  printf '%-44s %-16s %s (%ss)\n' "$scenario" "$program" "$status" "$runtime"
done

python3 "$ROOT/scripts/gate3_8/summarize_rtl_suite.py" \
  --status "$RESULTS" --output "$ROOT/reports/gate3_8/rtl_test_matrix.csv"

exit "$overall"
