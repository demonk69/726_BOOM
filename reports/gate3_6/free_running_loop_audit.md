# Gate 3.6 Free-running Loop Audit

## Verdict

The free-running `while (true)` loop is not the source of the 37936-LUT difference between `synth_core_step_top` and `boom_core_top`.

Direct controls:

| Top | Loop | LUT | FF | BRAM | Cycle-wrapper Instances |
|---|---|---:|---:|---:|---:|
| `synth_core_step_top` | none | 45350 | 12111 | 12 | 0 retained wrappers |
| `boom_core_ncycle_n1_top` | one call | 83353 | 16808 | 16 | 1 |
| `boom_core_ncycle_n8_top` | fixed trip count 8 | 83383 | 16812 | 16 | 1 |
| `boom_core_top` | infinite | 83286 | 16611 | 16 | 1 |

## Audit Items

| Question | Finding | Evidence | Classification |
|---|---|---|---|
| Is `boom_core_step` duplicated by the loop? | No. One `boom_core_cycle_io` instance exists for N1/N2/N4/N8/while; `boom_core_step` is inlined once into it. | N-cycle top reports and solution logs | REPORT_DIRECT + XFORM_LOG |
| Is the fixed loop unrolled? | No. Reports show trip counts 2/4/8, `Pipelined=no`; no unroll message exists; LUT is flat. | `ncycle_resource_scaling.csv` and N-cycle reports | REPORT_DIRECT + XFORM_LOG |
| Does the infinite loop add a large FSM? | No. The while top is 67 LUT smaller than N1. Its outer reported multiplexer cost is 128 LUT. | `boom_core_top_csynth.rpt` | REPORT_DIRECT |
| Are stream FIFOs duplicated? | No. Direct step and product top both report 335 FIFO LUT and 495 FIFO FF. | Gate 3.4 reports | REPORT_DIRECT |
| Is state duplicated? | No source or hierarchy evidence. One static `BoomCoreState` exists in the selected top and is passed by reference. | `src/boom_core_top.cpp`, `src/boom_core_step.cpp` | SOURCE_ANALYSIS |
| Is the wrapper retained? | Yes. Product-interface N1/N2/N4/N8/while tops retain one `boom_core_cycle_io`; direct diagnostic top does not. | Top instance tables | REPORT_DIRECT |
| Is automatic scalarization different? | Yes. Direct diagnostic top reports 342 automatic partitions; retained-wrapper tops report zero. | solution logs | XFORM_LOG |
| Are helpers cloned? | No clone messages. Helper count is one per module, but helper LUT grows through RAM-port mux interfaces. | helper diff and logs | REPORT_DIRECT + XFORM_LOG |
| Does `ap_ctrl_none` create the gap? | Not by itself. N1 also uses `ap_ctrl_none` and has the same high-cost wrapper form; comparison to direct diagnostic remains confounded by interface/reset/boundary. | source and N1 report | REPORT_DIRECT + SOURCE_ANALYSIS |
| Does reset cause the storage elaboration gap? | Yes. Removing only the state reset pragma reduces the same product top from 83286 to 45602 LUT, restores 342 automatic partitions, and reduces the cycle implementation by exactly 37684 LUT. This is storage/mux/helper expansion, not a second core instance. | T4 product csynth and recursive report comparison | REPORT_DIRECT |

## Exact Delta Accounting

Recursive HLS category accounting closes the 37936-LUT delta:

| Category | Delta LUT |
|---|---:|
| Multiplexer | +27681 |
| Helper instances | +9765 |
| Memory | +472 |
| Expression | +18 |
| FIFO | 0 |
| Total | +37936 |

The dominant multiplexer growth is resettable state-field RAM port arbitration in decode, rename, issue, execute, and ROB state. T3 rejects function-boundary retention as the trigger; T4 directly isolates whole-state reset elaboration. There is no duplicated core execution logic.
