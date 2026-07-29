# Gate 3.9 Artifact Integrity

The Gate 3.8 generated RTL, matrix, failing traces, accepted normal traces, reset audit, and synthesis report were copied into `baseline_artifacts/` before the Gate 3.9 implementation. They were used read-only for comparison.

`source_hashes_before.txt` captures the pre-change source set. `source_hashes_after.txt` captures the final implementation, testbench, directive, and runner set. Unchanged core modules and directives retain their original hashes. Intended source changes are limited to reset integration, merged-source generation/HLS source lists, the RTL stress testbench, and Gate 3.9-specific tests and runners.

Generated evidence was produced from the F1 conservative synthesis RTL in `variants/F1_FINE_GRAIN_RESET/conservative_rtl/`. XSim scenarios were executed serially against one snapshot to avoid build-directory races.
