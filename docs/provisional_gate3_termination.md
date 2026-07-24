# Provisional Gate 3 Termination Scope

Provisional Gate 3 uses `PREFIX` mode, not complete-program mode.

The BOOM standalone traces execute generated reset/boot code first, then jump into the loadmem-backed test program at `0x80000000`. HLS is normalized to begin directly at `0x80000000` because the BOOM boot ROM instructions are not present in the loadmem images and are recorded as unavailable instructions in the standalone trace.

The comparison stops before the first dynamic unsupported store, the retired `SD` to `tohost` at `0x80000080`. BOOM uses that store for real program termination. HLS decodes `SD`, but `src/lsu.cpp` leaves `lsu_module` as a no-op, so HLS cannot execute or retire the same store-to-`tohost` termination path.

This means a Provisional Gate 3 pass can only mean that the loaded-program architectural prefix before the `tohost` store matches. It must not be reported as full program equivalence, official Gate 3, or strict cycle equivalence.

Artifacts:

- Common dynamic subset: `reports/equivalence/provisional_gate3/common_instruction_subset.csv`
- Normalized trace schema: `reference/equivalence_trace_schema.json`
- BOOM normalizer: `scripts/normalize_boom_trace.py`
- HLS normalizer: `scripts/normalize_hls_trace.py`
- Diff runners: `scripts/run_architectural_diff.sh`, `scripts/run_event_order_diff.sh`, `scripts/run_normalized_cycle_diff.sh`
