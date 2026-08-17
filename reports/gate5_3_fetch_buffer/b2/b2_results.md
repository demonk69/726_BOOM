# Gate 5.3 B2 Fetch Buffer Integration Results

## Status

- `GATE5_3_B2_FETCH_BUFFER_INTEGRATION_VERIFIED=true`
- B2 is accepted. B3 packet construction was not started.
- Canonical path: modular sources through `boom_core_top`; raw `boom_core_step` and `src/boom_all.cpp` were not acceptance inputs.

## Functional Evidence

- Focused native integration: 169 checks, 0 failures, including all five mandatory RVC/cross-word semantics. See `test_integrity_audit.md`.
- Persistent random integration: 256 seeds x 4096 cycles, with zero drops, duplicates, ordering errors, stale side effects, or post-flush old entries. See `logs/random.log`.
- Decoupling: occupancy 1 through 8 observed; capacity backpressure occurred only at full; all five stall scenarios passed. See `decoupling_metrics.csv` and `throughput_analysis.md`.
- Focused generated RTL: 47/47 PASS, with explicit generated-RTL coverage of all five mandatory semantics. See `rtl_test_matrix.csv`.
- Full-core mixed-RVC native and csim: 11/11 each. Full-core generated RTL: 11/11 PASS from current modular-source RTL. See `phase_d/generation_provenance.md` and `phase_d/full_core_rtl_matrix.csv`.

## Preservation Evidence

- Phase E closed with current-run evidence: Gate 5.1 33/33; RVC 65,536/65,536 and Decode cross 38,551/38,551; W3 400/400; W4E 95/95 directed and 128/128 random; Gate 3.9 49/49; RV64M native/csim/full-core/focused; W3 11/11 and W4 20/20 focused RTL; full-program 10/10; partial-order 7/7.
- See `regression_after.md` and `regression/`.

## Canonical PPA

- Phase F fresh sequential canonical csynth: 11/11 tops PASS; all reports use Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, 10.00 ns target, and `PipelineType=no`.
- `boom_core_top`: 129885 LUT, 29194 FF, 16 BRAM_18K, 3 DSP, 6.341 ns.
- Gate 5.2 delta for `boom_core_top`: +2982 LUT (+2.350%), +1290 FF (+4.623%), unchanged BRAM, DSP, and estimated period.
- See `phase_f_ppa.csv` and `phase_f_summary.md`.

## Guardrails

- Source freshness passed for all 32 included modular/header inputs; `src/boom_all.cpp` and generated `src/boom_core_merged.cpp` were excluded from the source manifest.
- No DATAFLOW, false-dependence, explicit complete-array-partition, or `CORE_CYCLE` pipeline directive was introduced. `CORE_CYCLE` remains non-pipelined.
- `src/decode.cpp` retained its task-start hash and existing warning format.
- Fetch Buffer remains depth 8, AUTO storage, control-only reset, registered no-bypass dequeue, and one-wide canonical producer enqueue. No FTQ, predictor, ICache, or four-wide packet constructor was added.
