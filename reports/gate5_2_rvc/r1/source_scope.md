# Gate 5.2 R1 Source Scope

## In Scope

- Standalone RV64C decompressor interface and implementation.
- Canonical `RvcDecodeResult` and `decompress_rvc(uint16_t)` API.
- Exhaustive 65,536-halfword supported/unsupported/reserved/non-RVC tests.
- 228 directed encoding-class and boundary checks.
- 38,294 current-core-supported expansion-to-Decode cross checks.
- Independent `synth_rvc_top` and Frontend preservation csynth.
- Exactly-once canonical merged-source inclusion.
- R1 scripts, documentation, and evidence.

## Frozen And Out Of Scope

- No Frontend integration, halfword cursor, parcel carry, or mixed stream work.
- No changes to Frontend, Decode, state, common types, or backend modules.
- No width, capacity, completion, PRF, wakeup, bypass, trace, or interface change.
- No predictor, ICache, Fetch Buffer, or FTQ implementation.
- No product-top behavior change and no full-core synthesis claim.
- No `CORE_CYCLE` pipeline, `DATAFLOW`, false-dependence, or complete-partition directive.
- No expected trace or reference artifact changes.
- `src/boom_all.cpp` remains excluded, pre-existing dirty, and untouched.

R1 verifies only the contained standalone decompressor requested by the Gate 5.0
PPA-risk plan. R2 remains blocked on later RVC fetch integration and
mixed 16/32-bit, cross-word, PC, redirect, and alignment verification.
