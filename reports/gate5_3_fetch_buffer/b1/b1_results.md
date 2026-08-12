# Gate 5.3 B1 Standalone Fetch Buffer Results

## Status

- `GATE5_3_B1_FETCH_BUFFER_VERIFIED=true`
- `GATE5_3_B1_STANDALONE_ONLY=true`
- `GATE5_3_B1_CANONICAL_DEPTH=8`
- `GATE5_3_B1_CANONICAL_STORAGE=AUTO`
- `GATE5_3_B1_RESET_POLICY=CONTROL_ONLY`
- `GATE5_3_B1_READY_FOR_FRONTEND_INTEGRATION=false`

B1 verifies the standalone packet-to-one-wide Fetch Buffer boundary. It does not connect the buffer to canonical Frontend and does not claim complete Gate 5.3 packet construction or integration.

## Contract

- Compile-time depths 2, 4, 8, and 16 are supported; the configured SmallBoom candidate remains 8 entries.
- A packet contains four lanes and a four-bit effective valid mask. Valid lanes compact in ascending lane order.
- Packet acceptance is atomic. A packet backpressures unless all valid lanes fit.
- One entry can dequeue per call. A same-call dequeue makes its slot available to enqueue.
- Flush has priority over enqueue and dequeue and clears all queue control state.
- Reset clears head, tail, and count. Payload RAM is not reset in the accepted implementation.
- Stored metadata is PC, canonical instruction, fetch ID, RVC indication, exception, and exception cause.

## Functional Evidence

`scripts/gate5_3/run_b1_native_tests.sh` passes at every supported depth:

| Depth | Directed | Persistent random |
|---:|---|---|
| 2 | PASS, 62 checks | PASS, 256 seeds x 4096 cycles |
| 4 | PASS, 96 checks | PASS, 256 seeds x 4096 cycles |
| 8 | PASS, 162 checks | PASS, 256 seeds x 4096 cycles |
| 16 | PASS, 294 checks | PASS, 256 seeds x 4096 cycles |

Each random depth executes 10,485,760 protocol/state checks and covers successful enqueue/dequeue, atomic backpressure, flush, and head/tail wrap. Detailed counts are in `logs/random_d*.log`; the matrix is `native_test_matrix.csv`.

Canonical depth-8 generated RTL passes 10/10 directed cases through `scripts/gate5_3/run_b1_rtl.sh`. The cases cover runtime reset, sparse mask compaction, one-wide drain, full, backpressure, same-cycle capacity reuse, FIFO order, head/tail wrap, flush priority, and zero-mask behavior. Evidence is `rtl_test_matrix.csv` and `logs/rtl_xsim.log`.

## Focused HLS PPA

Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, 10 ns target. All variants retain `Pipelined=no`; no DATAFLOW, false-dependence directive, or complete array partition is used. Values are HLS estimates, not post-route utilization or STA.

| Variant | LUT | FF | BRAM_18K | DSP | Period ns | Latency cycles |
|---|---:|---:|---:|---:|---:|---:|
| depth 2 auto | 3,204 | 399 | 0 | 0 | 7.162 | 0 |
| depth 4 auto | 16,957 | 2,782 | 0 | 0 | 5.834 | 1 |
| depth 8 auto | 1,179 | 846 | 0 | 0 | 4.574 | 1-4 |
| depth 16 auto | 1,181 | 597 | 4 | 0 | 5.134 | 1-4 |
| depth 8 LUTRAM | 1,179 | 652 | 0 | 0 | 4.574 | 1-4 |
| depth 8 BRAM | 1,153 | 458 | 8 | 0 | 5.134 | 1-4 |
| depth 8 payload reset | 1,362 | 865 | 0 | 0 | 4.574 | 2-11 |

The non-monotonic depth 2/4 auto results are retained rather than normalized away; they reflect HLS mux/register implementation choices. Depth 8 is the accepted architectural candidate. Auto and forced LUTRAM have equal LUT/timing, while auto avoids an implementation directive. Forced BRAM saves 26 LUT versus auto but consumes 8 BRAM and increases estimated period by 0.560 ns, so it is rejected. Payload reset adds 183 LUT, 19 FF, and raises worst-case latency from 4 to 11 cycles, so control-only reset is accepted.

The full machine-readable sweep, including report paths, is `csynth_sweep.csv`.

## Scope Audit

- `src/frontend.cpp` is unchanged.
- Decode, dispatch, commit, backend queues, FTQ, predictor, ICache, LSU, and FPU are unchanged.
- `src/boom_all.cpp` is unchanged by B1 and excluded from this work.
- No canonical full-core PPA claim is made because the standalone buffer is not integrated.

## Reproduction

```bash
bash scripts/gate5_3/run_b1_native_tests.sh
bash scripts/gate5_3/run_b1_csynth_sweep.sh
bash scripts/gate5_3/run_b1_rtl.sh
```
