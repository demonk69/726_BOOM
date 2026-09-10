# Gate 5.4 PF4AR Artifact Provenance Repair

PF4AR aligns PF4 artifact metadata with `docs/gate_artifact_policy.md` without
changing product source, tests, runners, generated canonical source, PPA, or
functional verdicts.

```text
PF4_ARTIFACT_PROVENANCE_REPAIR=PASS
READY_FOR_PF4_COMMIT=true
CURRENT_DURABLE_CANONICAL_SYNTH_EVIDENCE_SUFFICIENT=true
CURRENT_DURABLE_FULL_CORE_RTL_EVIDENCE_SUFFICIENT=true
CANONICAL_SYNTH_RERUN_PERFORMED=false
FULL_CORE_RTL_REGEN_PERFORMED=false
PF4_DURABLE_ARTIFACT_PATHS_VERIFIED=true
PF4_DURABLE_ARTIFACT_HASHES_VERIFIED=true
PF4_EPHEMERAL_BUILD_PATHS_CLASSIFIED=true
PF4_EPHEMERAL_BUILD_PATH_MISSING_COUNT=10
PF4_CANONICAL_SYNTH_PROVENANCE_SOURCE_HASH_MATCH=true
PF4_FULL_CORE_RTL_PROVENANCE_SOURCE_HASH_MATCH=true
PF4_CANONICAL_SYNTH_RAW_REPORT_MATCH=9/9
FULL_CORE_LUT=210914
FULL_CORE_FF=46551
FULL_CORE_BRAM=16
FULL_CORE_DSP=3
FULL_CORE_PERIOD_NS=6.341
SYNTH_PREDICTOR_FOUNDATION_LUT=684
SYNTH_PREDICTOR_FOUNDATION_FF=465
PF4AR_PRODUCT_SOURCE_CHANGED=false
PRODUCT_COMMIT_BIM_TRAINING_INTEGRATED=false
PF5_FILES_CREATED=0
```

## Evidence Decision

Canonical synthesis is durably established by the canonical synthesis CSV,
the parsed full-core summary, the full Vitis `boom_core_top` log, its independent
runner timing file, and the matching canonical merged-source hash. Full-core
RTL is durably established by the source freshness manifest, generation
provenance, 12-program result matrix, mandatory fault evidence, and committed
execution traces.

The deleted HLS solution, generated RTL, simulator state, and preservation-run
paths are `REPRODUCIBLE_WORKSPACE`. They are retained only as historical run
locations and are not required to exist at commit. No synthesis or RTL rerun
was needed for this metadata repair.

`durable_artifact_hashes.csv` hashes every selected `CURATED_EVIDENCE` file
except itself. The checksum inventory is intentionally self-excluded to avoid
a circular hash; the eventual Git tree anchors that file.
