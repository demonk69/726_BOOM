# PF3R PPA Root Cause And Repair

## Result

Canonical PF3R synthesis reports `198482 LUT / 45293 FF / 16 BRAM / 3 DSP /
6.341 ns`. This closes all three PF3 PPA blockers:

- Period is below the 6.5 ns gate and exactly matches the PF2 estimate.
- BRAM returns from 17 to the PF2 count of 16.
- LUT growth from PF2 is 17675, or 9.775%, below the 10% review boundary.

## BRAM Root Cause

The additional PF3 BRAM was not the canonical FTQ table. It was the ROB-resident
`MicroOp::ftq_generation`: logical memory
`state_rob_entries_uop_ftq_generation_U`, inferred as a 32 x 32 single-port
memory and merged into `state_rob_entries_uop_inst_RAM_AUTO_1R1W`. The PF3
synthesis binding records one BRAM for this memory.

PF3R stores the generation in a dedicated 32 x 32 ROB sidecar, forces only that
sidecar to `RAM_2P/LUTRAM`, and restores it at the load-completion, normal-commit,
and exception-commit consumers. The canonical PF3R report contains
`state_rob_ftq_generations_U` as
`state_rob_ftq_generations_RAM_2P_LUTRAM_1R1W` with zero BRAM; the original
ROB-generation memory is absent. The canonical 211-bit FTQ payload remains in
LUTRAM.

## Timing Root Cause

PF3's integrated `frontend_module` state `ST_8` was 6.73 ns and drove the
6.739 ns full-core estimate. Its combinational dependency connected Fetch Buffer
dequeue/capacity decisions to legacy predictor `resp_ready`, predictor state
advance, a 64-bit request-token match, and matching-response state updates.

PF3R adds a read-only predictor `peek()` and separates observation of the old
pending response from `step()` state advance. The canonical product path is also
compile-time specialized through `frontend_mode_module<true>`, removing the
runtime product/legacy mode mux from `boom_core_step`. The repaired Frontend's
largest state is 4.93 ns; the unchanged execute path again sets the 6.341 ns
full-core estimate.

## LUT Repair

Compile-time product specialization removed dead legacy-mode logic from the
canonical product path. Expressing the frozen FTQ references at their actual
5-bit index, 1-bit lane, and 2-bit halfword-offset widths produced the final E5
candidate. No architecture contract, queue depth, generation width, atomicity,
or prediction feature scope changed.

## Evidence

- PF3 report: `/tmp/boom_hls/boom_hls_gate5_4_pf3_canonical_boom_core_top/solution_module/syn/report/boom_core_top_csynth.rpt`
- PF3 schedule: `/tmp/boom_hls/boom_hls_gate5_4_pf3_canonical_boom_core_top/solution_module/.autopilot/db/frontend_module.verbose.sched.rpt`
- PF3R report: `/tmp/boom_hls/boom_hls_gate5_4_pf3r_canonical_boom_core_top/solution_module/syn/report/boom_core_top_csynth.rpt`
- PF3R schedule: `/tmp/boom_hls/boom_hls_gate5_4_pf3r_canonical_boom_core_top/solution_module/.autopilot/db/frontend_mode_module_true_s.verbose.sched.rpt`
- Canonical summary: `reports/gate5_4_pf3r_canonical/module_csynth_summary.csv`
