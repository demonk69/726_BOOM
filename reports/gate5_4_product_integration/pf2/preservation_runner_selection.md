# Preservation Runner Selection

`GATE5_3_PRESERVATION_HARNESS=B3I_PACKET_AWARE_ACCEPTED`

The accepted `scripts/gate5_3/run_b3i_random.sh` and `fetch_packet_2lane_random_tests.cpp` ran 256 x 4096 with every counter zero. The B2 scalar-producer test was not used for the Gate 5.3 verdict. Runner source lists were updated only to link canonical P1/P2 dependencies.

`PRESERVATION_RUNNER_SELECTION_FIXED=true`

The old R2 focused RVC harness reports 1248 failures because it reuses product state across cases while resetting only legacy Frontend fields; after PF2, Predictor response state is also product state. Its expected semantics were not modified and it is recorded as `NON_VERDICT_DIAGNOSTIC` with reason `HARNESS_LIFECYCLE_VERSION_MISMATCH`, not PASS. Canonical RVC behavior passes PF2 directed/random, full-core native and CSim 11/11, and fresh full-core generated RTL 11/11.

The same versioning restriction applies to rebuilding frozen Gate 3.9 RTL with the current HEAD `boom_core_rtl_harness.sv`: the RTL exports a 192-bit IMEM request while that harness declares 128 bits. The resulting 0/49 zero-commit timeouts under `gate3_9_preservation/` are retained as `NON_VERDICT_DIAGNOSTIC` harness incompatibility evidence, not classified as architectural failures or PASS. Previously accepted Gate 3.9 and B3I matrices remain 49/49. Fresh PF2 RTL reset evidence is the PF1 exception matrix 8/8 and PF2 focused reset/redirect coverage.
