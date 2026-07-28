# Gate 3.6 Top-Level Architecture

## Verdict

Gate 3.6 status: `TOP_LEVEL_DELTA_EXPLAINED_NO_ACCEPTED_OPTIMIZATION`.

The 37936-LUT difference between the Gate 3.4 direct diagnostic top and the accepted product top is not duplicated core logic and is not caused by the free-running loop. The dominant cause is Vitis HLS 2021.2 elaboration of the explicitly reset persistent `BoomCoreState`: the whole-state reset prevents the 342 automatic partitions seen in the direct diagnostic top and produces larger state-memory mux and helper-port cones.

The reset directive is required product behavior. Removing it reduces LUT by 37684 but removes the synthesized mid-run hardware reset contract, so that diagnostic result is rejected and the Gate 3.3 conservative configuration remains accepted.

## Top Semantics

| Top | Role | Control | Interface | State | Loop | Reset |
|---|---|---|---|---|---|---|
| `synth_core_step_top` | direct diagnostic | `ap_ctrl_hs` | FIFO plus `seed_pc`/`observable` | independent static state | none | no explicit whole-state reset |
| `boom_core_step_top` | one-call product-interface control | `ap_ctrl_none` | AXIS plus product status outputs | independent static state | none | whole-state HLS reset |
| `boom_core_ncycle_n1_top` | attribution only | `ap_ctrl_none` | product interface | independent static state | one call | whole-state HLS reset |
| `boom_core_ncycle_n2_top` | attribution only | `ap_ctrl_none` | product interface | independent static state | fixed N=2 | whole-state HLS reset |
| `boom_core_ncycle_n4_top` | attribution only | `ap_ctrl_none` | product interface | independent static state | fixed N=4 | whole-state HLS reset |
| `boom_core_ncycle_n8_top` | attribution only | `ap_ctrl_none` | product interface | independent static state | fixed N=8 | whole-state HLS reset |
| `boom_core_top` | accepted product top | `ap_ctrl_none` | AXIS plus product status outputs | one static `BoomCoreState` | free-running | whole-state HLS reset |

`synth_core_step_top` and `boom_core_top` call the same in-place `boom_core_step` transition, but they are not interface- or reset-equivalent tops. `boom_core_step_top` and the N1 wrapper are the valid controls for separating one call from the free-running product loop.

## Resource Closure

| Comparison | LUT Delta | Interpretation |
|---|---:|---|
| accepted `boom_core_top` minus direct `synth_core_step_top` | +37936 | observed top gap |
| accepted `boom_core_top` minus T4 no-reset product top | +37684 | reset-induced elaboration isolated on the same product top |
| T4 no-reset product top minus direct diagnostic top | +252 | residual interface/wrapper difference |
| accepted product top minus accepted cycle wrapper | +469 | small outer interface/control shell |
| T4 no-reset product top minus T4 no-reset cycle wrapper | +469 | outer shell remains unchanged by reset experiment |
| accepted product top minus one-call product top | -67 | free-running loop does not increase area |

The accepted-to-no-reset 37684-LUT delta closes exactly inside the cycle implementation:

| Recursive Category | Accepted | No Reset | Reset-Induced Delta |
|---|---:|---:|---:|
| helper instances | 44302 | 34561 | +9741 |
| state memory | 730 | 258 | +472 |
| multiplexer | 37688 | 10217 | +27471 |
| total | 82720 | 45036 | +37684 |

The original direct-step/product recursive comparison closes its 37936-LUT delta as +27681 multiplexer, +9765 helper instances, +472 memory, +18 expression, and 0 FIFO LUT.

## Elaboration Findings

- Exactly one static `BoomCoreState` exists in each selected top; helpers receive it by reference. There is no `next_state` copy or second product state owner.
- Helper instance counts are identical between direct and product synthesis. Vitis HLS reports zero function clones; helper LUT grows because resettable state fields cross helper/RAM port interfaces.
- The direct diagnostic and T4 no-reset product runs report 342 automatic partitions. Accepted product and N-cycle resettable runs report zero.
- T3 force-inlining removes the retained `boom_core_cycle_io` module but still reports zero partitions and increases full-core LUT to 87388. Function-boundary retention alone is therefore not the cause.
- Direct diagnostic and product reports both account for 335 FIFO LUT and 495 FIFO FF. Stream adapters contribute zero to the 37936-LUT category delta.
- Control FSM LUT is small relative to the gap. The accepted free-running top is 67 LUT smaller than the one-call product control.
- No fixed N-cycle loop was unrolled or pipelined. N1/N2/N4/N8 remain within 30 LUT.

## N-Cycle Evidence

| Top | LUT | FF | BRAM_18K | DSP | Period | Partitions |
|---|---:|---:|---:|---:|---:|---:|
| N1 | 83353 | 16808 | 16 | 3 | 5.898 ns | 0 |
| N2 | 83379 | 16810 | 16 | 3 | 5.898 ns | 0 |
| N4 | 83381 | 16811 | 16 | 3 | 5.898 ns | 0 |
| N8 | 83383 | 16812 | 16 | 3 | 5.898 ns | 0 |
| free-running | 83286 | 16611 | 16 | 3 | 5.898 ns | 0 |

N2/N4/N8 have report-visible trip counts 2/4/8 with `Pipelined=no`. No unroll transformation is reported and resources do not scale with N.

## Experiment Decisions

| Experiment | Decision | Reason |
|---|---|---|
| T1 state ownership | `NOT_RUN_NO_STRUCTURAL_DEFECT` | accepted source already has one state owner and reference-only state passing |
| T2 output consolidation | `NOT_RUN_NO_STRUCTURAL_DEFECT` | scalar outputs already have one write point; event streams are event-gated; FIFO delta is zero |
| T3 function boundaries | `REJECTED_PPA` | all regressions pass, but inline top grows to 87388 LUT and still has zero partitions |
| T4 reset consolidation diagnostic | `REJECTED_RESET_SEMANTICS` | 45602 LUT exposes the cause but removes required hardware reset behavior |
| T5 stream adapter | `NOT_RUN_NO_DELTA_TO_TARGET` | FIFO LUT/FF are identical and the small 469-LUT outer shell is stable |

No combination experiment is permitted because no semantics-preserving single-variable experiment passed PPA acceptance.

## Accepted Configuration

Gate 3.3 conservative `boom_core_top` remains accepted:

- 83286 LUT
- 16611 FF
- 16 BRAM_18K
- 3 DSP
- 5.898 ns estimated period
- `CORE_CYCLE` pipeline disabled
- whole-state hardware reset retained

Strict BOOM cycle equivalence remains `INSUFFICIENT_EVIDENCE`. `M009` remains `PARTIALLY_VERIFIED`.

## Readiness

| Flag | Value | Reason |
|---|---|---|
| `READY_FOR_WIDE_ISSUE_IMPLEMENTATION` | false | top gap is explained, but no lower-area configuration was accepted and full multi-lane issue/execute equivalence is not established |
| `READY_FOR_CORE_PIPELINE_EXPERIMENT` | true | accepted top structure is stable, no duplicate core/state logic was found, and conservative csynth is repeatable; this permits only a separate gated experiment |
| `READY_FOR_OFFICIAL_GATE_3` | false | original Chipyard/FESVR/DRAMSim simulator environment remains unavailable |

## Evidence

- `reports/gate3_6/top_resource_delta.md`
- `reports/gate3_6/free_running_loop_audit.md`
- `reports/gate3_6/ncycle_resource_scaling.md`
- `reports/gate3_6/rtl_instance_diff.csv`
- `reports/gate3_6/ram_instance_diff.csv`
- `reports/gate3_6/helper_instance_diff.csv`
- `reports/gate3_6/inlining_clone_inventory.csv`
- `reports/gate3_6/control_fsm_inventory.csv`
- `reports/gate3_6/variants/T3_FUNCTION_BOUNDARIES/`
- `reports/gate3_6/variants/T4_RESET_CONSOLIDATION/`
