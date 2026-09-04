# PF2 Baseline Manifest

- Git root: `/home/lab_726/boom/hls_boom`
- Branch: `gate3.8-rtl-verification`
- Accepted HEAD: `8434489d61c99fa12896d80b038c58440b244de1`
- PF1 PPA baseline: 171540 LUT, 33704 FF, 16 BRAM, 3 DSP, 6.341 ns.
- Dirty baseline: preserved. No existing dirty file was reverted.
- `src/boom_all.cpp`: excluded from implementation, builds, synthesis, and evidence.
- PR0 preservation contract: `GATE5_3_B3I_ACCEPTED_CONTRACT` using `scripts/gate5_3/run_b3i_random.sh`.

Frozen PF0 decisions are `AFTER_CANONICAL_FETCH_PACKET_BUILD_BEFORE_PREDICTOR_REQUEST_OR_FETCH_BUFFER_ADMISSION`, per-packet earliest conditional request, required wait state, no-CFI bypass, static JAL bypass, frozen final admission mask, and separate Frontend/Predictor generations.
