# Gate 3.2 Results

Status: `BASELINE_CSYNTH_PASS`

Gate 3.2 closed the post-LSU Vitis HLS baseline csynth timeout for the conservative baseline. The accepted baseline does not enable `CORE_CYCLE` pipelining and does not add aggressive PPA directives.

## Verdicts

| Item | Result |
|---|---|
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| HLS C++ complete traces vs frozen Gate 3.2 baseline | 5/5 byte-identical |
| Vitis HLS csim complete traces vs frozen Gate 3.2 baseline | 5/5 byte-identical |
| BOOM vs HLS full loaded-program architectural diff | 10/10 PASS |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations |
| Module diagnostic csynth | 9/9 PASS |
| Finite `boom_core_step_top` csynth | PASS |
| Baseline `boom_core_top` csynth | PASS |
| Performance pipeline experiment | TIMEOUT after 15 minutes |
| Official Gate 3 | BLOCKED by missing original Chipyard/FESVR/DRAMSim path |

## Root Cause Closed

The failed post-LSU baseline synthesis was dominated by Vitis HLS transformation expansion of this per-cycle whole-state copy pattern:

```cpp
BoomCoreState next_state = state;
...
state = next_state;
```

The old baseline also had a hardcoded `#pragma HLS PIPELINE II=1` on `CORE_CYCLE`. Together those forced HLS to materialize and auto-partition many `next_state.*` temporaries, including ROB/IQ/LSU and branch snapshot arrays.

Gate 3.2 removes the whole-state copy, updates the persistent state in the existing serialized cycle order, and gates the core-loop pipeline pragma behind `BOOM_HLS_ENABLE_CORE_PIPELINE` so it is not part of the accepted baseline.

## Synthesis Results

| Mode | Top | Pipeline | Status | Runtime | Peak Memory | Estimated Period | LUT | FF | BRAM_18K | DSP |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| finite step top | `boom_core_step_top` | disabled | PASS | 44.09s | 1520876 KB | 5.898 ns | 40696 | 16182 | 16 | 3 |
| baseline core | `boom_core_top` | disabled | PASS | 45.56s | 1521072 KB | 5.898 ns | 40625 | 15985 | 16 | 3 |
| performance pipeline | `boom_core_top` | enabled | TIMEOUT_15_MIN | - | - | - | - | - | - | - |

Baseline report: `boom_hls_gate3_2_baseline/solution_baseline/syn/report/boom_core_top_csynth.rpt`.

Step-top report: `boom_hls_gate3_2_step_top/solution_step_top/syn/report/boom_core_step_top_csynth.rpt`.

## Module Csynth

All diagnostic module tops passed. Summary: `reports/gate3_2/module_csynth_summary.csv`.

## Preserved Behavior

The refactor preserved the frozen complete traces byte-identically for both native HLS C++ and Vitis HLS csim. The BOOM-vs-HLS full loaded-program architectural diff remains `10/10 PASS` for the minimal LSU subset.

## Limits

- This does not claim strict BOOM cycle equivalence.
- This does not implement full BOOM LSU/cache/MMU/TLB/PTW/miss/replay/AMO/LRSC behavior.
- This does not implement FPU, branch predictor, TileLink, or L2.
- This does not close the pipeline-enabled PPA experiment.
- Official Gate 3 remains blocked by missing original Chipyard generated sources, `libfesvr`, `libdramsim`, and RISC-V ELF/binutils tooling.

## Artifacts

| Artifact | Path |
|---|---|
| Regression summary | `reports/gate3_2/regression_after.md` |
| Timeout/root-cause analysis | `reports/gate3_2/csynth_timeout_analysis.md` |
| Baseline csynth summary | `reports/gate3_2/baseline_csynth_summary.md` |
| Step-top csynth summary | `reports/gate3_2/step_top_csynth_summary.md` |
| Top-mode synthesis summary | `reports/gate3_2/top_csynth_modes.csv` |
| Module csynth summary | `reports/gate3_2/module_csynth_summary.csv` |
| State update architecture | `docs/state_update_architecture.md` |
| Synthesis architecture | `docs/synthesis_architecture.md` |
