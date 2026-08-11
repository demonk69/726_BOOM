# Gate 5.1R R1 Native/ROB-Fill Diagnosis

## Scope And Method

- Current tree: `e065a4a`, whose Frontend implementation is `c6e1d04`.
- Accepted pre-Gate 5.1 ancestor: `c67ce8b`, read from the required existing worktree `/tmp/opencode/boom_gate41_baseline`.
- Instrumentation: `tb/differential/gate5_1_r1_rob_fill_trace.cpp`. It reads public native state and stream activity only. No product implementation, HLS directive, RTL wrapper, or `src/boom_all.cpp` was changed for R1.
- The harness reproduces the exact directed-test setup for six calls and separately runs a 128-cycle always-ready native Frontend stream in both trees.

Evidence is in `rob_fill_cycle_trace.csv`, `frontend_cycle_trace.csv`, `baseline_rob_fill_cycle_trace.csv`, `baseline_frontend_cycle_trace.csv`, and `baseline_comparison.csv`.

## Required Answers

1. **Maximum ROB occupancy:** 32 entries in both versions. The test initializes all 32 entries valid before cycle 0; it does not fill the ROB by executing 33 instructions.
2. **Expected occupancy:** 32 entries. It is already reached at cycle 0 in both versions.
3. **First divergence from the accepted baseline:** cycle 0. Both versions emit request `(addr=0x10040, fetch_id=0)`. The fixture's preloaded response has epoch 0. The current request has epoch `0xffffffff`, so Gate 5.1 drains it as stale; the baseline has no epoch validation and accepts it.
4. **Where progress differs:** response acceptance. Request generation and ROB occupancy are identical. The current implementation intentionally accepts one fewer response because the fixture did not establish a matching outstanding request. Consequently Decode/Rename/dispatch do not receive a uop. There is no early commit and no ROB allocation in either version because the ROB is already full.
5. **Fixed-cycle timeout only:** no. The test performs exactly one `boom_core_step` and immediately checks retained dispatch state. More importantly, the injected response is invalid under the Gate 5.1 identity contract.
6. **Six-times observation window:** no. Cycles 1-5 retain a real outstanding request, but the mismatched response was already drained in cycle 0 and the fixture supplies no matching response. The result cannot recover with time alone.
7. **Final repair constraint:** timeout expansion is neither necessary nor valid. The W3 fixture must first issue/capture a request and return a response with its address, fetch ID, and epoch, while preserving the full ROB setup and backpressure assertion.

## A/B Metrics

| Metric | Current | `c67ce8b` baseline |
|---|---:|---:|
| ROB fixture cycles to first fetch | 0 | 0 |
| ROB fixture cycles per fetch | N/A, one request | N/A, one request |
| ROB fixture cycles to first dispatch | N/A | 0 |
| ROB fixture cycles per dispatch | N/A | N/A, one dispatch |
| ROB fixture maximum occupancy | 32 | 32 |
| ROB fixture cycles to expected occupancy | 0 | 0 |
| Always-ready native cycles per fetch | 2 | 2 |
| Always-ready native cycles per response | 2 | 2 |
| Always-ready native cycles to first dispatch | 1 | 1 |
| Always-ready native cycles per dispatch | 2 | 2 |

The always-ready native stream is cycle-for-cycle identical through the measured 128 cycles. The Gate 5.1 six-`ap_clk` focused interval is therefore an HLS/top scheduling issue layered over a pre-existing two-native-cycle Frontend recurrence, not the cause of the W3 399/400 result.

## R1A Bubble Analysis

- **Response acceptance and next request:** accepting a matching response clears `request_sent` in the same call, but linearly building the held entry ends with an unconditional return. The next request is not prepared until the following architectural call. This is a legal but unnecessary one-cycle recurrence.
- **Decode consume and new holding transition:** `frontend_module` runs before `decode_module` and computes `stalled` from current Decode/Rename valid state. It cannot observe a same-call downstream consume as next-state readiness, so a response cannot replace a consumed held entry in that case.
- **Request readiness:** request eligibility uses current state (`request_sent`, `fetch_packet_valid`, and current downstream valids), not computed next state.
- **Holding valid clear:** a held packet is cleared when current downstream state is free; this does not add another interval in the measured normal stream, but it is not expressed as an explicit consume/replace next-state transition.
- **Fetch ID/epoch/stale drain:** the valid always-ready stream shows no extra wait from identity checks. Stale responses are drained immediately. The fixture failure is exactly the intended stale-response rejection.

## Classification

- W3 399/400 root cause: **`R1_OTHER`**, specifically a legacy directed-test fixture invalidated by the required Gate 5.1 response identity contract.
- Native Frontend throughput condition for R3: **`R1_ARCHITECTURAL_BUBBLE`**, because response acceptance cannot prepare the next request in the same architectural call and readiness is not based on next state.
- Not selected: `R1_TIMEOUT_ONLY`, `R1_REQUEST_STATE_BUG`, `R1_RESPONSE_STATE_BUG`, or `R1_HOLDING_STATE_BUG` for the W3 failure.

R1 is complete. No implementation or directive change was made before this conclusion.
