# Critical Path

## Baseline

```text
T0R_BASELINE_PERIOD_NS=6.341
T0R_BASELINE_PATH_DELAY_NS=6.34
T0R_BASELINE_STARTPOINT=state_int_rf_bank1_load_9_at_include/boom_state.hpp:410
T0R_BASELINE_ENDPOINT=unsigned_64x64_multiply_ret_in_execute_mul
T0R_BASELINE_CRITICAL_PATH=PRF_BANK1_READ_1.24NS_TO_RS2_MUX_0.574NS_TO_PHI_0NS_TO_UNSIGNED_MULTIPLY_4.53NS
```

Fresh raw evidence is in `raw_evidence/baseline_execute_module.verbose.sched.rpt:1598-1603`.

## Accepted Candidate

Candidate A consumes the operand values already resolved and published by Issue for MUL-family uops. Its multiply state is 4.53 ns and no longer contains the PRF read or 0.574 ns operand mux. A source-equivalent `|divisor| == 1` rewrite reduces `divider_accept` from 5.734 ns to 4.830 ns. The full-core critical path transfers to `build_fetch_packet`:

```text
T0R_FINAL_PERIOD_NS=6.071
T0R_FINAL_CRITICAL_PATH=BUILD_FETCH_PACKET_APPEND_PARCEL_4.27NS_PLUS_CONTROL_AND_PC_MUX
MULTIPLY_PATH_TRANSFERRED=true
DIVIDER_ACCEPT_PATH_TRANSFERRED=true
```

Fresh raw evidence is in `raw_evidence/A5_divider_accept.verbose.sched.rpt:216-230` and `raw_evidence/A5_build_fetch_packet.verbose.sched.rpt:457-469`.
