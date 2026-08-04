# Gate 4.0 W4D Multi Writeback

Historical W4D checkpoint result: **PASS**. W4E final RTL and random-128 had not yet run; this is not the current final product claim.

## Semantics

- Integer write topology is exactly `WritebackEvent[2]`; commit remains width one and wakeup/bypass remain width three.
- Selection is wrap-safe ROB age, then LSU load, MEM execute, INT execute. Two distinct nonzero destinations write in one cycle. A third writer is retained and the measured maximum wait is two cycles.
- Same destination/equal value coalesces to one physical write and completes both selected owners. Same destination/different value performs no write/wakeup and becomes a precise validation fault (`cause=0x100`): pending events are consumed, participants become nonbusy exceptions, commit emits one exception trace, `io_trap` asserts, and reset recovers. Sticky status and measured conflict/fault counters remain visible.
- ROB busy clears only for a selected/coalesced write. x0 never writes. Allocation ownership, lane-2 exclusion, branch kill, oldest exception fence, and load-response ownership remain enforced.
- Pending control resolves before surviving writers are validated. Mispredict recovery removes younger writers before comparison; a correct branch preserves cross-boundary writers. A conflict suppresses all wakeup, bypass, and writeback publication in its detection cycle. Publication and service use one allocation-valid oldest-exception scan over pending events and live ROB entries, so retained younger events remain fenced on all later cycles while older events may progress. Operand order is x0, bypass, ready PRF.
- Clean runs record zero dropped completions and writebacks. A directed duplicate-held-source probe records exactly one completion drop and removes the duplicate execute copy while the retained event proceeds.

## Verification

- Product cumulative suites: **15/15 PASS, 400/400 checks, 197 runs**.
- W4A **19/19**, W4B **17/17**, W4C wakeup **14/14**, W4D bypass **14/14**, W4D writeback **13/13**, W4D completion **6/6**, independent oracle C++ **10/10**.
- W3 random: **100/100 seeds**, 6,400 cycles; W2 random: **64/64 seeds**, 2,048 cycles. Measured dropped writebacks **0**, duplicate physical writes **0**, conflicts **1**, validation faults **1**, consumed fault events **2**, and safe de-duplications **1**.
- Reset default/pipelined **14/14 each**; C++ versus csim trace pairs **7/7**; normalized architecture/event/cycle **21/21**; full-program **10/10**; partial order **7/7**.
- Directed peaks: completion accepts **2**, ROB completes **2**, PRF writes **2**, wakeups **3**, bounded wait **2**. Random peaks: ROB completes **3**, PRF writes **2**, wakeups **2**.
- All seven C++ and csim program cycle endpoints are unchanged from W4C. Event deltas are `accepted_uops +17`, `execute_events +17`, `completion_consumed +8`, `PRF writes +19`, `wakeups -57`, `ROB completes +23`, `commits +7`, over the same 6,400 cycles.

Evidence: `regression/w4d/w4d_metrics.csv`, `regression/w4d/w4d_event_cycle_deltas.csv`, `regression/w4d/w4d_cycle_deltas.csv`, and `regression/w4d/product_full/`.

## Synthesis

Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, 10 ns: **7/7 targets PASS**. All reports are unpipelined at the top level; the isolated physical write stage meets target/final II=1.

| Top | Est. ns | LUT | FF | BRAM_18K | DSP |
|---|---:|---:|---:|---:|---:|
| `synth_issue_top` | 4.445 | 17,301 | 4,821 | 0 | 0 |
| `synth_execute_top` | 5.009 | 1,816 | 384 | 4 | 3 |
| `synth_completion_top` | 5.474 | 37,206 | 10,715 | 4 | 0 |
| `synth_rob_top` | 3.746 | 29,085 | 8,195 | 5 | 0 |
| `synth_lsu_top` | 5.772 | 43,005 | 12,094 | 7 | 0 |
| `synth_core_step_top` | 5.976 | 101,338 | 24,564 | 16 | 3 |
| `boom_core_top` | 5.976 | 107,693 | 24,918 | 16 | 3 |

Generated PRF evidence in all three product views is `RAM_T2P_BRAM_1R1W` with independent `we0` and `we1`; no complete partition and `INT_PHYS_REGS=52`. All six product status ports are generated outputs. Product XSim build/elaboration and one compatibility run pass. The independent oracle synthesizes and passes ten generated-RTL scenarios, including correct-branch cross-boundary conflict, fault plus unrelated-younger suppression, mispredict-killed non-conflict, and persistent exception-fence publication suppression across three cycles. Bind audit remains zero. Evidence is in `w4d_resource_summary.csv`, `w4d_oracle_resource_summary.csv`, `w4d_rtl_oracle_results.csv`, `w4d_bind_audit.csv`, `prf_after.csv`, `status_ports_after.csv`, and `w4d_review_fix_results.md`.

## Blockers

No identified W4D review blocker remains. Twelve pre-existing HLS 200-805 stream-depth warnings remain. Six parsed resource pragmas per target produce HLS 207-5523 deprecation warnings in Vitis 2021.2, but bind successfully and are guarded by generated RTL inspection. Official external Chipyard/FESVR equivalence remains unavailable. W4E remains intentionally deferred.
