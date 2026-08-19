# Frontend Architecture Review

Gate 5.0 F0 established that the canonical HLS Frontend is a minimal one-logical-outstanding external-IMEM adapter, not the configured/generated SmallBoom Frontend. Gate 5.1 added fetch ID, 32-bit epoch, expected-address matching, stale-response drain, architectural redirect ownership, fetch-fault propagation, and target-alignment faults. Gate 5.2 added the supported integer RV64C decompressor, retained 32-bit response parsing, and one cross-word carry. Gate 5.3 adds an eight-entry Fetch Buffer and two-lane packet admission. FTQ, dynamic prediction/RAS, and ICache remain absent.

The repository-generated SmallBoom target proves fetch width 4, decode width 1, 8 fetch bytes, 8 Fetch Buffer entries, 16 FTQ entries, 8 branch tags, RVC, a 32-entry RAS, composed loop/TAGE/BTB/micro-BTB/BIM prediction, and a 16 KiB four-way ICache. Exact source-level policy for several predictor structures remains UNKNOWN and is not required by Gate 5.1.

Gate 5.1R completes final acceptance. A non-product verification wrapper exposes the mandatory redirect/ownership/fault/hold/runtime-reset state without duplicating Frontend next-state logic, and generated RTL passes 33/33. The canonical next-state repair removes the native two-call request bubble and restores W3 to 400/400; S0 with no scheduling directive is accepted, while S1/S3 remain rejected. See `reports/gate5_1_frontend/gate5_1_results.md`.

Gate 5.2 R3 closes the protected `C.EBREAK`, RV64 `C.SRLI shamt[5]`, and `C.JALR` decode/link gaps. `GATE5_2_RVC_VERIFIED=true` for the supported integer RV64C scope. Production branch recovery invalidates carry and advances the request epoch, while successful cross-word assembly consumes its response exactly once.

## Gate 5.3 Final Freeze

The canonical Fetch Buffer stores eight complete instruction entries with AUTO storage and CONTROL_ONLY reset. Frontend owns request/response identity, cross-word carry, packet construction, and redirect invalidation. A matched 32-bit response plus optional carry produces at most one packet with up to two complete instructions. Multi-response packet aggregation is disabled. Packet admission is all-or-nothing; Decode and Dispatch remain one wide.

B3I full-core csynth is 135953 LUT, 33373 FF, 16 BRAM_18K, 3 DSP, and 6.341 ns. Against B2 this is `+6068` LUT (`+4.672%`) and `+4179` FF (`+14.315%`) with no BRAM, DSP, or period change. `PACKET_WIDTH_2_ACCEPTED` captures both complete RVC parcels and improves response utilization and burst fill, but makes no end-to-end IPC claim.

The next released activity is `FTQ_PREREQUISITE_REVIEW`, not implementation. It must define fetch-bundle metadata ownership, index lifetime, named consumers, redirect pruning, and backpressure before FTQ or predictor state is added. Predictor-first would orphan history/update state; ICache-first lacks a refill/invalidation contract. `READY_FOR_FTQ_IMPLEMENTATION=false`, `READY_FOR_PREDICTOR_IMPLEMENTATION=false`, and `READY_FOR_ICACHE_IMPLEMENTATION=false`.

See `reports/gate5_3_fetch_buffer/final/gate5_3_final_results.md`.
