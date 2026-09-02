# Implementation Status

Gate 1 update: M003, M004, M006 are closed for the implemented integer/control subset.

Gate 2 update: Gate 2.5 corrected the standalone generated-model traces; they are finite loadmem-backed traces with retired-store `tohost` termination. Official Chipyard simulator/toolchain dependencies remain absent. `READY_FOR_PROVISIONAL_GATE_3=true`, `READY_FOR_GATE_3=false`.

Provisional Gate 3 update: BOOM-vs-HLS csim architectural prefixes pass for five loadmem-backed programs, comparing 45 commits before the store-to-`tohost` boundary. Event-order and normalized-cycle diffs fail because the HLS prefix trace is serialized and does not reproduce BOOM branch-resolution interleaving.

Gate 3.1A update: corrected event comparison uses per-uop partial order. The old global event-order failure is a validation-method false positive for the current traces: 8 legal reorder events, 0 real exposed partial-order violations, commit order matches. RAW/WAR/WAW timing remains insufficient-signal because issue, wakeup, rename-source, and complete events are missing.

Gate 3.1C update: minimal integer LSU/store-to-`tohost` support is implemented and validated for the directed subset. BOOM-vs-HLS full loaded-program architectural diff passes 10/10 across native HLS C++ and Vitis csim complete traces. Baseline Vitis csynth after LSU timed out during HLS transformations before producing a report.

Gate 3.2 update: conservative post-LSU synthesis is closed. `BoomCoreState next_state = state` was removed from `boom_core_step.cpp`, baseline `CORE_CYCLE` pipelining is disabled by default and macro-gated for experiments, frozen complete traces remain byte-identical, module diagnostic csynth passes, finite step-top csynth passes, and baseline `boom_core_top` csynth passes in 45.56s with a 5.898 ns estimated period. The separate pipeline-enabled performance experiment still times out after 15 minutes.

Gate 3.3 update: branch recovery is implemented for the supported single-lane integer/minimal-LSU subset. HLS now allocates branch tags, propagates branch masks, snapshots/restores the integer rename map, tracks per-branch physical destination allocation lists, rolls back the free list on mispredict, rebuilds busy state from valid busy ROB entries, clears resolved branch bits, and kills younger ROB/IQ/execute/LDQ/STQ state. Existing regressions remain passing, branch snapshot directed/random tests pass, merged source compiles, module diagnostic csynth passes, finite step-top csynth passes, and conservative `boom_core_top` csynth passes in 71.69s with a 5.898 ns estimated period.

Gate 3.4 update: resource attribution and module csynth baselines are complete. Gate 3.4 adds analysis scripts and attribution-only synthesis tops, not architectural behavior. `boom_core_top` still synthesizes with 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, and 5.898 ns estimated period. No LUT-reducing optimization candidate is accepted.

Gate 3.5 update: six branch-recovery structural experiments completed with full functional, trace, architectural-diff, partial-order, and product csynth gates. No variant reached the 10% LUT-reduction threshold, so Gate 3.3 remains accepted.

Gate 3.6 update: the 37936-LUT direct-diagnostic/product-top gap is explained. N1/N2/N4/N8 and free-running resources are flat, no loop unroll or state/core duplication exists, and FIFO LUT delta is zero. Removing only whole-state HLS reset reduces the same product top by 37684 LUT and exposes 342 automatic partitions, but violates required hardware reset semantics. T3 inlining increases LUT. Status is `TOP_LEVEL_DELTA_EXPLAINED_NO_ACCEPTED_OPTIMIZATION`; the accepted configuration is unchanged.

Gate 3.7 update: `CORE_CYCLE` has real loop-carried state, control, memory-port, and stream dependencies. P0 exactly reproduces the accepted baseline. P1 applies outer-loop pipeline without an II target and times out at 900 seconds during Presyn 2 transformations, before scheduling, achieved-II calculation, RTL, or resource reporting. P2-P6 and local pipeline experiments are not run because their prerequisite report gates are unmet. Status is `CORE_CYCLE_PIPELINE_TRANSFORMATION_TIMEOUT_NO_SYNTHESIS_CANDIDATE`.

Gate 3.8 update: accepted conservative RTL passes 42/45 XSim scenarios, including all tested AXIS backpressure cases and seven normal-program C++/csim/RTL architectural comparisons. `R2_RESET_ROB_NONEMPTY`, `R6_RESET_BRANCH_RECOVERY`, and `P0_RESET_AND_BRANCH_MISPREDICT` fail because runtime reset restarts generated control but retains most architectural state, including the frontend PC and ROB/rename/IQ storage. Status is `RTL_RESET_MISMATCH`.

Gate 3.9 update: fine-grain reset closes M014 with XSim 49/49 and becomes the accepted 47999-LUT baseline.

Gate 3.10 update: the real critical path is LSU load extraction at 5.90 ns. R1 local reset pipeline reaches II=1 and XSim 49/49 but is rejected because reset latency worsens and normal cycles change. L1-L5 are illegal dependencies/handshakes/recovery paths. No pipeline candidate is accepted.

Gate 4.0 W1 update: fixed three-lane issue and execute-result interfaces are verified while implemented acceptance remains one uop per cycle.

Gate 4.0 W2 update: the shared implemented IQ can generate one oldest-ready MEM grant and one oldest-ready INT grant in the same cycle. Independent lane backpressure retains unaccepted grants, but conservative execute intake remains one. Status is `W2_DUAL_SELECTION_VERIFIED`, not dual execution.

Gate 4.0 W3 update: fixed MEM/INT lanes accept and execute concurrently, retain two pending completions, and serialize completion/writeback with allocation-identity, branch-kill, reset, and blocked-dispatch safeguards. Status is `W3_DUAL_EXECUTE_ACCEPTANCE_VERIFIED`.

Gate 4.0 W4 update: the false-dependence pragma is removed and replaced by a two-replica, 52-bit-LVT logical integer PRF. Software/random and RTL matrices pass, including the no-partition two-real-`boom_core_step` diagnostic; source-bound W4E final csynth passes all seven requested tops, both bank write enables are generated, and the write stage reaches II=1. `W4_MULTI_WRITEBACK_VERIFIED=true`, `READY_FOR_COMPLETE_M_EXTENSION=true`, `READY_FOR_FRONTEND_IMPLEMENTATION=false`, and `READY_FOR_FULL_LSU_IMPLEMENTATION=false`.

Gate 4.1 M1 update: exact decode and INT-compatible issue classification are verified for all 13 RV64M operations. Required legal, `rd=x0`, illegal near-miss, and base OP/OP-32 collision vectors pass 42/42. W3/W4 software, random, frozen trace, architecture, and partial-order preservation pass; conservative csynth passes 3/3 with `boom_core_top` at 6.025 ns and `CORE_CYCLE` unpipelined. `M1_RV64M_DECODE_VERIFIED=true` and `READY_FOR_M2_MUL_FAMILY=true`; arithmetic execution is not yet implemented or claimed.

Gate 4.1 M3C update: complete RV64M directed/random, native/csim/generated-RTL full-core, reset, and canonical synthesis evidence is accepted. `GATE4_1_RV64M_VERIFIED=true`.

Gate 5.1R final update: native/UBSan and focused generated RTL 33/33 pass; the next-state repair sustains one request per native architectural call and restores W3 to 400/400. Gate 3.9 full-core RTL is 49/49; RV64M native/csim/full-core RTL are 15/15 each; focused M3C/W3/W4 are 30/30, 11/11, and 20/20. Canonical csynth is 9/9 with `boom_core_top` at 124317 LUT, 27513 FF, 16 BRAM, 3 DSP, and 6.341 ns, with `CORE_CYCLE Pipelined=no`. `GATE5_1_FOCUSED_RTL_VERIFIED=true`, `GATE5_1_THROUGHPUT_BLOCKER=false`, `GATE5_1_PPA_BLOCKER=false`, `GATE5_1_FRONTEND_FOUNDATION_VERIFIED=true`, `READY_FOR_GATE5_2_RVC=true`, and `READY_FOR_FRONTEND_IMPLEMENTATION=true`. `READY_FOR_FULL_LSU_IMPLEMENTATION=false`, `READY_FOR_OFFICIAL_GATE_3=false`, `M009=PARTIALLY_VERIFIED`, and `M014=VERIFIED` remain unchanged.

Gate 5.2 R1 update: the standalone integer RV64C decompressor uses canonical `RvcDecodeResult`/`decompress_rvc`, rejects compressed FP memory forms as current-core unsupported, passes 228 directed checks, all 65,536 parcel comparisons, and 38,294 supported expansion-to-Decode checks. It is not connected to Frontend. `GATE5_2_R1_RVC_DECOMPRESSOR_VERIFIED=true` and `READY_FOR_GATE5_2_R2_RVC_FETCH=true`; neither full Gate 5.2 nor Fetch Buffer readiness is claimed.

Gate 5.2 R2 update: the one-wide Frontend now consumes mixed 16/32-bit streams, retains an aligned response word, advances by PC+2/PC+4, assembles cross-word 32-bit instructions, and preserves exact request identity, redirects, reset, backpressure, and fault attribution. Focused native passes 4,111 assertions across 414 cases, 256x2048 random passes with successful and faulted cross-word assembly, focused RTL passes 58/58, and mixed full-core native/csim/RTL pass 10/10 each. Ten final csynth tops pass; `boom_core_top` is 126798 LUT, 28492 FF, 16 BRAM, 3 DSP, and 6.341 ns with `Pipelined=no`. One `C.EBREAK`, 256 `C.SRLI shamt5`, and 31 `C.JALR` forms remain explicit illegal cause 2. `GATE5_2_R2_RVC_FETCH_VERIFIED=true`, `READY_FOR_GATE5_2_R3_DECODE_GAP_CLOSURE=true`, `GATE5_2_RVC_VERIFIED=false`, and `READY_FOR_FETCH_BUFFER=false`.

Gate 5.2 R3 update: `C.EBREAK`, RV64 `C.SRLI shamt[5]`, and `C.JALR` decode/link semantics are closed for the supported integer RV64C scope. Focused and full-core evidence passes and `GATE5_2_RVC_VERIFIED=true`.

Gate 5.3 final update: standalone FIFO parameterization, scalar integration/decoupling, packet architecture review, and two-lane packet implementation are accepted. The canonical configuration is depth 8, AUTO storage, CONTROL_ONLY reset, packet width 2, 32-bit IMEM response, and one-wide Decode/Dispatch with one logical outstanding request and no multi-response aggregation. B3I full-core csynth is 135953 LUT, 33373 FF, 16 BRAM, 3 DSP, and 6.341 ns. `GATE5_3_FETCH_BUFFER_VERIFIED=true`, `GATE5_3_PPA_BLOCKER=false`, and `READY_FOR_GATE5_4_PREREQUISITE_REVIEW=true`; FTQ, Predictor, and ICache implementation readiness remain false.

Gate 5.4 P1 update: standalone stateless CFI predecode recognizes conditional/JAL/JALR including canonical RV64C expansions, computes exact direct targets, classifies x1/x5 calls/returns, and selects the earliest two-lane packet CFI. Directed 994/994, RV64C exhaustive 65,536 with zero false positives, one-million random words, 256x4096 random packets, generated RTL 51/51, and scalar/packet csynth pass. The scalar module is 639 LUT, 0 FF/BRAM/DSP, 2.442 ns, zero-cycle combinational, and unpipelined. It is not connected to product Frontend; Predictor/BIM/BTB/RAS/GHR/FTQ/ICache remain unimplemented.

Gate 5.4 P2 update: the standalone predictor foundation is verified with a 256-entry, 2-bit PC-indexed BIM, LUTRAM storage, lazy valid initialization, commit-qualified generation/metadata-validated updates, and forwarded same-index update values. Directed 5,008/5,008, canonical predecode/RVC composition 10,620/10,620, random 75,497,472 checks, and generated LUTRAM RTL 164/164 pass with zero errors. Standalone HLS estimates are 684 LUT, 465 FF, 0 BRAM/DSP, and 2.989 ns; best-case top transaction latency is 3 and minimum II is 4, while the architectural API remains request call N/response call N+1. Product Frontend/FTQ/Execute/Commit are unchanged, no full-core PPA is claimed, `GATE5_4_P2_PREDICTOR_FOUNDATION_VERIFIED=true`, and `READY_FOR_GATE5_4_F1_FTQ_FOUNDATION=true`; product predictor/FTQ readiness and implementation remain false.

Gate 5.4 F1 update: a standalone parameterized FTQ is verified at depths 8/16/32/64 with one entry per accepted nonempty Fetch Packet, lane-mask live tracking, ordered reclaim, redirect owner retention, runtime reset, 32-bit allocation-generation stale protection, and bit-exact retention of the independent 32-bit P2 predictor generation and BIM metadata. Directed/bounded exhaustive testing passes 275,944,648 checks, persistent random passes 587,202,560 checks over 256x16,384 cycles at four depths, P1/P2/FTQ composition passes 11,080 checks, and generated LUTRAM RTL passes 193/193. Canonical depth-32 LUTRAM estimates are 3006 LUT, 1358 FF, 0 BRAM/DSP, and 3.788 ns. Product sources are unchanged; product predictor/FTQ integration remains false pending PF0 exception/recovery review.

Gate 4.0 W3 source scope: acceptance covers the modular `src/*.cpp` implementation plus generated `src/boom_core_merged.cpp` and public `src/boom_core_top.cpp` only. `src/boom_all.cpp` is a legacy, non-canonical, unreferenced monolithic snapshot; active build, test, RTL-generation, and csynth scripts do not consume it. It is excluded from source manifests and acceptance without deletion or rewrite. Pre-existing dirty tracked logs and backup logs are excluded non-deliverables and are not evidence.

## Implemented And Tested

- Frontend request/response FSM with one outstanding request, fetch ID, 32-bit epoch, expected-address matching, stale drain, redirect priority/ownership, fetch faults, alignment checks, and verified next-state request replacement.
- Integer RV64C parcel fetch/decompression for the supported subset, including retained-word reuse, one cross-word carry, `C.EBREAK`, RV64 `C.SRLI shamt[5]`, and `C.JALR` link semantics.
- Eight-entry complete-instruction Fetch Buffer with AUTO storage, CONTROL_ONLY reset, atomic two-lane packet admission, flush/backpressure handling, and one-wide dequeue.
- RV64 integer ALU subset used by directed tests.
- JAL, JALR, and conditional branches with always-not-taken baseline redirect behavior.
- Integer rename map/free-list/stale-pdst commit release for single dispatch lane.
- ROB allocate, complete, commit, full backpressure, and wrap behavior for current tests.
- Shared implemented issue queue dispatch/select/compact with fixed MEM/INT selection lanes and up to two accepted execute inputs.
- Three retained completion slots (LSU load, MEM execute, INT execute) with oldest-first service, up to three ROB completions/wakeups, and two PRF writes.
- Persistent radix-2 DIV/DIVU/REM/REMU and W variants in the INT lane, with held response, allocation identity, branch kill, and fine-grained reset.
- CSR cycle/instret and ECALL success/trap subset.
- Commit trace output for directed tests.
- Minimal integer LSU path for LB/LBU/LH/LHU/LW/LWU/LD and SB/SH/SW/SD in the current conservative single-lane path.
- Committed store-to-`tohost` termination via LSU `DmemRequest`.
- Gate 3.3 branch tag allocation, branch mask propagation, rename-map snapshot/restore, free-list allocation-list rollback, resolved-mask clear, and selective younger-state kill for the supported subset.

## Partial Or Mismatch

- Branch recovery is implemented for the supported HLS subset, but strict BOOM event/cycle timing is not verified and the original Chisel source checkout is unavailable for direct inspection.
- M004 JALR redirect is verified only as a concrete Gate 1 functional test; Gate 3.3 branch recovery is tracked separately under M009.
- Full BOOM IssueWidth=3 execution is not implemented; W3 supports fixed MEM/INT dual acceptance only, and the FP lane remains unsupported.
- The three-port busy-table wakeup/bypass network is verified for the supported subset but is not equivalent to BOOM's full speculative/replay network; recovery rebuilds busy state from still-valid busy ROB entries.
- Exception and flush handling are coarse compared with BOOM.
- Cycle timing is not verified against BOOM.
- Conservative no-pipeline csynth remains stable. Gate 3.7 confirms the full-cycle pipeline transformation still fails to close even without an II target; no pipelined schedule or minimum II is available.
- Pin-level AXIS backpressure and runtime reset pass the Gate 3.9 49-case matrix. Local pipeline experiments do not replace that accepted baseline.

## Not Implemented

- Full BOOM LSU behavior, DCache, ICache, MMU/Sv39, TLB, PTW, cache miss/replay, AMO/LRSC, and full memory-ordering semantics.
- FPU and FP issue/register-read/writeback paths.
- Product-integrated branch predictor, BTB, BIM, TAGE, RAS, and FTQ. P2's BIM predictor is standalone only.
- TileLink, L2, interrupts, full CSR file, privilege transitions.

## Latest Verification

- Directed tests: 25 passed, 0 failed.
- Gate 1 regressions: 13 passed, 0 failed.
- Vitis HLS 2021.2 csim: 5 passed, 0 failed.
- Merged HLS compilation unit: C++ compile PASS.
- Gate 2.5 BOOM standalone traces: PASS_LOADMEM, five traces checked with 40/40/41/42/42 commits and no max-cycle truncation.
- Provisional Gate 3: ARCHITECTURAL_PREFIX_PASS, five BOOM-vs-HLS csim prefixes checked with 8/8/9/10/10 commits; event-order and normalized-cycle checks FAIL as expected for the serialized HLS subset.
- Gate 3.1A partial-order analysis: PARTIAL_MATCH, old event-order failure classified `VALIDATION_METHOD_FALSE_POSITIVE`, 8 legal reorders, 0 real exposed partial-order violations.
- Gate 3.1C minimal LSU: 14/14 LSU tests pass; complete HLS C++ and Vitis csim traces reach retired `SD` to `tohost`; full loaded-program architectural diff passes 10/10.
- Gate 3.1C csynth: TIMEOUT after 30 minutes during HLS transformations; no updated `csynth.rpt` produced.
- Gate 3.2 baseline csynth: PASS for `boom_core_top`, 45.56s runtime, 1521072 KB peak memory, 5.898 ns estimated period, 40625 LUT, 15985 FF, 16 BRAM_18K, 3 DSP.
- Gate 3.2 finite step-top csynth: PASS for `boom_core_step_top`, 44.09s runtime, 1520876 KB peak memory, 5.898 ns estimated period.
- Gate 3.2 module diagnostic csynth: PASS for all 9 tops in `reports/gate3_2/module_csynth_summary.csv`.
- Gate 3.2 performance pipeline csynth: TIMEOUT after 15 minutes with `BOOM_HLS_ENABLE_CORE_PIPELINE=1`; not part of the accepted baseline.
- Gate 3.3 branch snapshot tests: directed 30/30 PASS; random 2/2 PASS with seed `0x3a33b007`.
- Gate 3.3 regression preservation: directed 25/25 PASS; Gate 1 regressions 13/13 PASS; minimal LSU 14/14 PASS; HLS C++ complete traces 5/5 byte-identical; Vitis HLS csim complete traces 5/5 byte-identical; full-program architectural diff 10/10 PASS; partial-order analysis has 8 legal reorders and 0 real exposed violations.
- Gate 3.3 merged source: `src/boom_core_merged.cpp` regenerated and C++ compile PASS.
- Gate 3.3 module diagnostic csynth: PASS for all 9 tops in `reports/gate3_3/module_csynth_summary.csv`.
- Gate 3.3 finite step-top csynth: PASS for `boom_core_step_top`, 69.78s runtime, 1521136 KB peak memory, 5.898 ns estimated period, 83353 LUT, 16808 FF, 16 BRAM_18K, 3 DSP.
- Gate 3.3 baseline csynth: PASS for conservative `boom_core_top`, 71.69s runtime, 1520472 KB peak memory, 5.898 ns estimated period, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP.
- Performance pipeline csynth: NOT_RUN through Gate 3.4; prior Gate 3.2 `BOOM_HLS_ENABLE_CORE_PIPELINE=1` timeout remains deferred.
- Gate 3.4 module baseline: PASS for all 12 requested attribution/module tops in `reports/gate3_4/module_baseline.csv`.
- Gate 3.4 conservative `boom_core_top` csynth: PASS, 69.33s runtime, 1521180 KB peak memory, 5.898 ns estimated period, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP.
- Gate 3.4 regressions: directed 25/25 PASS; Gate 1 13/13 PASS; minimal LSU 14/14 PASS; branch snapshot directed 30/30 PASS; branch snapshot random 2/2 PASS; HLS C++ and Vitis csim complete traces are each 5/5 byte-identical to Gate 3.3; full-program architectural diff 10/10 PASS.
- Gate 3.4 optimization status: no accepted optimization; Gate 3.3 remains accepted PPA configuration.
- Gate 3.5 single-variable structural optimization status: B1, B4, C1, D1, D4, and D4-IQ completed full functional/trace/csynth gates. Only D4 reduced LUT, from 83286 to 82789, but this is below the 10% acceptance threshold. No Gate 3.5 optimization accepted.
- Gate 3.5 regressions: every variant preserved directed 25/25 PASS, Gate 1 13/13 PASS, minimal LSU 14/14 PASS, branch snapshot directed 30/30 PASS, expanded branch snapshot random 42/42 PASS, HLS C++/Vitis csim byte-identical traces, full-program architectural diff 10/10 PASS, and partial-order 8 legal reorders with 0 real violations.
- Gate 3.6 top audit: direct `synth_core_step_top` is 45350 LUT; product N1/N2/N4/N8 are 83353/83379/83381/83383 LUT; free-running `boom_core_top` is 83286 LUT. Fixed and infinite loops retain one cycle wrapper, report no unroll, and do not scale resources.
- Gate 3.6 T3: force-inlined cycle boundary passes all regressions but synthesizes at 87388 LUT, 22117 FF, 16 BRAM_18K, 3 DSP, and 5.898 ns with zero automatic partitions; `REJECTED_PPA`.
- Gate 3.6 T4 attribution: removing only product state reset synthesizes at 45602 LUT, 12119 FF, 12 BRAM_18K, 3 DSP, and 5.898 ns with 342 automatic partitions; `REJECTED_RESET_SEMANTICS`.
- Gate 3.6 restored baseline regressions: directed 25/25, Gate 1 13/13, LSU 14/14, branch directed 30/30, branch random 42/42, IQ 10/10, HLS trace 10/10 byte-identical, BOOM diff 10/10, partial-order 8 legal/0 real.
- Gate 3.6 readiness: `READY_FOR_WIDE_ISSUE_IMPLEMENTATION=false`, `READY_FOR_CORE_PIPELINE_EXPERIMENT=true` for a separate gated experiment only, and `READY_FOR_OFFICIAL_GATE_3=false`.
- Gate 3.7 P0 baseline: PASS in 71.40s, 1520716 KB peak memory, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, 5.898 ns.
- Gate 3.7 P1 no-II pipeline: TIMEOUT after 900.00s in Presyn 2; 65 implied complete-unroll markings, 57 completed unroll records, 8 variable-bound failures, 0 automatic partitions, no achieved II/report.
- Gate 3.7 P2-P6: NOT_RUN by required P1/P1-P5 report gates. No `SYNTHESIS_CANDIDATE` or `FUNCTIONALLY_VERIFIED_CANDIDATE` exists.
- Gate 3.7 final regressions: directed 25/25, Gate 1 13/13, LSU 14/14, branch 30/30+42/42, IQ 10/10, trace 10/10 byte-identical, BOOM diff 10/10, partial-order 8 legal/0 real.
- Gate 3.7 readiness: `READY_FOR_WIDE_ISSUE_IMPLEMENTATION=false`, `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION=false`, `READY_FOR_ACCEPT_PIPELINED_CONFIG=false`, `READY_FOR_OFFICIAL_GATE_3=false`.
- Gate 3.8 XSim matrix: 42/45 PASS. All 6 trace-backpressure, 7 IMEM, 9 DMEM, and 7 normal-program scenarios pass; reset cases R2/R6 and priority case P0 fail.
- Gate 3.8 normal traces: seven programs pass C++ versus Vitis csim versus generated RTL commit and `tohost` comparison.
- Gate 3.8 reset audit: frontend PC/validity, ROB, IQ, issued state, rename maps/free/busy state, branch snapshots, RF, execute results, and LSU queues are `RESET_INITIAL_ONLY`; status `RTL_RESET_MISMATCH`.
- Gate 3.8 conservative csynth: PASS with 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, and 5.898 ns; accepted configuration remains unchanged.
- Gate 3.8 readiness: `READY_FOR_WIDE_ISSUE_IMPLEMENTATION=false`, `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION=false`, `READY_FOR_OFFICIAL_GATE_3=false`.
- Official Chipyard/FESVR/DRAMSim trace: BLOCKED by missing original `/root/chipyard` generated-source path, `libfesvr`, `libdramsim`, and RISC-V ELF/binutils tools.
- Gate 4.0 W1: fixed issue/result lane interface PASS; conservative `boom_core_top` is 51558 LUT, 12802 FF, 12 BRAM_18K, 3 DSP, and 5.898 ns.
- Gate 4.0 W2 source verification: 177/177 recorded assertions PASS plus a 64-seed, 2048-cycle random differential campaign; 63 dual-grant cycles, 382 accepted, 424 retained, and 0 dropped generated grants.
- Gate 4.0 W2 generated RTL: dedicated issue selection 5/5 PASS and full-core XSim 49/49 PASS.
- Gate 4.0 W2 csynth: `boom_core_top` is 61760 LUT, 15213 FF, 12 BRAM_18K, 3 DSP, and 5.898 ns; this selection checkpoint is not the accepted PPA baseline.
- Gate 4.0 W2 readiness: `READY_FOR_DUAL_EXECUTE_ACCEPTANCE_EXPERIMENT=true`, `READY_FOR_PARTIAL_WIDE_ISSUE_MAX2=false`, and `READY_FOR_OFFICIAL_GATE_3=false`.
- Gate 4.0 W3 software: 15/15 suites, 400/400 checks, and 100x64 persistent random PASS with 0 dropped and 0 duplicate tokens.
- Gate 4.0 W3 generated RTL: focused dual-execute RTL 11/11 PASS; full-core XSim, normalized trace, and architecture comparison 49/49 PASS.
- Gate 4.0 W3 csynth: all five canonical targets PASS; `boom_core_top` is 68055 LUT, 16149 FF, 15 BRAM_18K, 3 DSP, and 5.898 ns. Values are HLS estimates, not post-route timing or accepted product PPA.
- At the Gate 4.0 W3 checkpoint, `READY_FOR_W4_MULTI_WAKEUP_WRITEBACK=true`, `READY_FOR_OFFICIAL_GATE_3=false`, M009 `PARTIALLY_VERIFIED`, and M014 `VERIFIED`; W4 was not yet present at that historical checkpoint.
- Gate 4.0 W4E software/random: 95/95 cumulative W4 directed checks, 128/128 seeds, 16,384 cycles, peak 3 completion sources, 2 writes, 3 wakeups, and 3 bypasses; zero dropped/duplicate/stale-side-effect/unexplained tokens.
- Gate 4.0 W4 RTL: focused W4 20/20, current W3 11/11, and full-core XSim/normalized architecture 49/49 PASS. The added diagnostic uses two real core steps without complete partitioning; modular product source and preserved product RTL hashes are unchanged.
- Gate 4.0 W4E source-bound csynth: 7/7 PASS at 10 ns on `xczu7ev-ffvc1156-2-e`; `boom_core_top` is 111869 LUT, 25094 FF, 16 BRAM_18K, 3 DSP, and 6.025 ns. `CORE_CYCLE` has no pipeline II; writeback target/final II=1.
- Gate 4.1 M1 decode: 42/42 focused vectors, W3 400/400, W4 directed 95/95, reset 14/14, W4 random 128/128, frozen traces 14/14 byte-identical, full-program diff 10/10, and conservative csynth 3/3 PASS. `boom_core_top` is 114088 LUT, 25719 FF, 16 BRAM_18K, 3 DSP, and 6.025 ns.
- Gate 4.0 final statuses: `M009=PARTIALLY_VERIFIED`, `M014=VERIFIED`, `READY_FOR_OFFICIAL_GATE_3=false`. Strict BOOM cycle equivalence is not claimed; official Chipyard/FESVR/DRAMSim validation remains externally unavailable.
