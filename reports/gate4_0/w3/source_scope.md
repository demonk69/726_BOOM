# Gate 4.0 W3 Source Scope

## Canonical Scope

W3 acceptance covers the modular implementation translation units consumed by the regression compile list and merged-source generator, the generated `src/boom_core_merged.cpp`, and the public `src/boom_core_top.cpp` top. Supporting headers, testbenches, scripts, and synthesis harness tops are inventoried where they directly produce W3 evidence, but they do not broaden the implementation claim.

`src/boom_all.cpp` is a legacy, non-canonical monolithic snapshot. It is not an input to the active build, test, regression, merged-source generation, RTL-generation, or csynth flows. It is retained for historical context, is not synchronized with the canonical modular implementation, and is explicitly excluded from W3 source hashes, evidence, and acceptance.

The canonical manifests are `source_hashes_after.txt`, `regression/source_hashes_after.txt`, `full_core_source_hashes.txt`, and `source_hashes.sha256`. The immutable W2 baseline manifest `source_hashes_before.txt` is historical evidence and is not rewritten; its inclusion of the old snapshot does not make that snapshot canonical for W3.

## Dirty-Log Exclusion

The pre-existing 2 modified tracked logs and 273 untracked backup logs recorded in `git_status_before.txt` are excluded, non-deliverable workspace state. The baseline recorded their paths but not their content hashes, so W3 makes no provenance claim about their current bytes. They are not evidence inputs and are absent from `artifact_manifest.csv`.
