# Gate 3.6 Results

Date: 2026-07-27

## Verdict

Status: `TOP_LEVEL_DELTA_EXPLAINED_NO_ACCEPTED_OPTIMIZATION`.

Gate 3.6 closes the 37936-LUT direct-step/product-top difference. The dominant source is whole-state reset elaboration in Vitis HLS 2021.2, not a duplicated core, free-running-loop unroll, FIFO duplication, or multiple persistent state copies. Removing the required product reset reduces LUT by 37684 and exposes 342 automatic partitions, but that variant is invalid. Gate 3.3 conservative `boom_core_top` remains accepted.

## Baseline

| Metric | Accepted Baseline |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| CORE_CYCLE pipeline | disabled |
| Whole-state reset | retained |

## Required Answers

| Question | Answer |
|---|---|
| 1. Are `synth_core_step_top` and `boom_core_top` semantically identical? | No. They execute the same in-place `boom_core_step` transition, but the diagnostic top uses `ap_ctrl_hs`/FIFO, `seed_pc`, `observable`, and no explicit whole-state reset. The product top uses AXIS, `ap_ctrl_none`, product status outputs, a persistent internal stream bundle, a free-running loop, and required whole-state reset. |
| 2. Exact source of the 37936-LUT difference | Whole-state reset-triggered storage elaboration is dominant. The same product top without only the state reset pragma is 45602 LUT, 37684 below accepted and only 252 above the direct diagnostic top. The resettable form suppresses 342 automatic partitions and expands RAM-port mux/helper cones. |
| 3. Free-running-loop incremental LUT | No positive increment. `boom_core_top` is 67 LUT smaller than the one-call product control, 83286 versus 83353 LUT. The outer loop/FSM does not scale the core. |
| 4. Persistent-state contribution | No large contribution by itself. T4 retains the same static persistent `BoomCoreState` and falls to 45602 LUT when only reset is removed. There is one state owner and no whole-state copy. |
| 5. Stream-adapter contribution | FIFO category delta is exactly 0: both direct and product reports contain 335 FIFO LUT and 495 FIFO FF. The complete product outer shell is 469 LUT above its cycle wrapper in both reset and no-reset runs, so it is not the 37936-LUT source. |
| 6. Reset-induced logic contribution | 37684 LUT, 4492 FF, and 4 BRAM_18K on the same product top. Inside the cycle implementation this is +9741 helper-instance LUT, +472 memory LUT, and +27471 multiplexer LUT, summing exactly to 37684. |
| 7. Helper clone contribution | Zero clones and identical helper counts. Direct versus accepted helper instances differ by 9765 LUT because the same helpers attach to different state/RAM-port elaborations; accepted versus no-reset isolates 9741 LUT of this growth to reset. |
| 8. N1/N2/N4/N8 scaling | 83353/83379/83381/83383 LUT. N2/N4/N8 add only 26/28/30 LUT over N1; free-running is 67 LUT below N1. |
| 9. Was any loop implicitly unrolled? | No. N2/N4/N8 reports show trip counts 2/4/8, `Pipelined=no`, no unroll transformation, one retained cycle wrapper, and flat resources. |
| 10. Is state or core logic duplicated? | No. One selected-top static state owner, reference-only state passing, one cycle wrapper, one instance of each helper, zero function-clone messages, and flat N-cycle scaling rule out duplication. |
| 11. Best single-variable variant | T4 no-reset diagnostic is 45602 LUT, -37684/-45.25%, but is rejected for reset semantics. The only semantics-preserving source experiment, T3, rises to 87388 LUT and is rejected by PPA. |
| 12. Final accepted configuration | Gate 3.3 conservative `boom_core_top`: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, 5.898 ns, pipeline disabled, reset retained. |
| 13. Gate 3.6 status | `TOP_LEVEL_DELTA_EXPLAINED_NO_ACCEPTED_OPTIMIZATION`. |
| 14. Readiness flags | `READY_FOR_WIDE_ISSUE_IMPLEMENTATION=false`; `READY_FOR_CORE_PIPELINE_EXPERIMENT=true` for a separate gated experiment only; `READY_FOR_OFFICIAL_GATE_3=false`. |

## Recursive Delta

| Category | Direct Step LUT | Accepted Product LUT | Delta |
|---|---:|---:|---:|
| Expression | 85 | 103 | +18 |
| FIFO | 335 | 335 | 0 |
| Helper instances | 34537 | 44302 | +9765 |
| Memory | 258 | 730 | +472 |
| Multiplexer | 10135 | 37816 | +27681 |
| Total | 45350 | 83286 | +37936 |

## N-Cycle Results

| Top | LUT | FF | BRAM | DSP | Period | Partitions |
|---|---:|---:|---:|---:|---:|---:|
| `boom_core_ncycle_n1_top` | 83353 | 16808 | 16 | 3 | 5.898 ns | 0 |
| `boom_core_ncycle_n2_top` | 83379 | 16810 | 16 | 3 | 5.898 ns | 0 |
| `boom_core_ncycle_n4_top` | 83381 | 16811 | 16 | 3 | 5.898 ns | 0 |
| `boom_core_ncycle_n8_top` | 83383 | 16812 | 16 | 3 | 5.898 ns | 0 |
| `boom_core_top` | 83286 | 16611 | 16 | 3 | 5.898 ns | 0 |

## Experiment Results

| Variant | Functional/Trace | LUT | FF | BRAM | DSP | Period | Verdict |
|---|---|---:|---:|---:|---:|---:|---|
| T1 state ownership | inherited direct evidence | - | - | - | - | - | `NOT_RUN_NO_STRUCTURAL_DEFECT` |
| T2 output consolidation | inherited direct evidence | - | - | - | - | - | `NOT_RUN_NO_STRUCTURAL_DEFECT` |
| T3 function boundaries | PASS / 10/10 byte-identical | 87388 | 22117 | 16 | 3 | 5.898 ns | `REJECTED_PPA` |
| T4 reset diagnostic | PASS_CSIM_ONLY / 10/10 byte-identical | 45602 | 12119 | 12 | 3 | 5.898 ns | `REJECTED_RESET_SEMANTICS` |
| T5 stream adapter | inherited direct evidence | - | - | - | - | - | `NOT_RUN_NO_DELTA_TO_TARGET` |

No combination was run because no semantics-preserving single-variable experiment passed acceptance.

## Regression Status

T3, T4, and the final restored accepted source each pass directed 25/25, Gate 1 13/13, LSU 14/14, branch directed 30/30, branch random 42/42, IQ 10/10, HLS C++/Vitis csim trace comparison 10/10 byte-identical, BOOM architectural diff 10/10, and partial-order comparison with 8 legal reorders and 0 real violations.

T4's C++/csim pass is not reset-equivalence evidence because the removed HLS pragma affects generated hardware rather than native C++ behavior.

## Status Preservation

- `M009=PARTIALLY_VERIFIED`
- Strict BOOM cycle equivalence: `INSUFFICIENT_EVIDENCE`
- Official Gate 3: blocked by missing original Chipyard/FESVR/DRAMSim environment
- No queue depth, branch count, state capacity, field width, interface, ISA scope, execution-unit count, or memory-system scope was reduced
- Rejected T3 and T4 source changes were restored
- Attribution-only N-cycle tops remain excluded from the product configuration

## Primary Artifacts

| Artifact | Path |
|---|---|
| Baseline manifest | `reports/gate3_6/baseline_manifest.md` |
| Semantic comparison | `reports/gate3_6/top_semantic_comparison.md` |
| Recursive resource delta | `reports/gate3_6/top_resource_delta.md` |
| Free-running-loop audit | `reports/gate3_6/free_running_loop_audit.md` |
| N-cycle scaling | `reports/gate3_6/ncycle_resource_scaling.md` |
| RTL instance diff | `reports/gate3_6/rtl_instance_diff.csv` |
| RAM instance diff | `reports/gate3_6/ram_instance_diff.csv` |
| Helper diff | `reports/gate3_6/helper_instance_diff.csv` |
| Inlining/clone inventory | `reports/gate3_6/inlining_clone_inventory.csv` |
| Control FSM inventory | `reports/gate3_6/control_fsm_inventory.csv` |
| Variant summary | `reports/gate3_6/variant_summary.csv` |
| Final regression | `reports/gate3_6/regression_after.md` |
| Architecture documentation | `docs/gate3_6_top_level_architecture.md` |
