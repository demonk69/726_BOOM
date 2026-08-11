# Frontend Architecture Review

Gate 5.0 F0 established that the canonical HLS Frontend is a minimal one-instruction, one-logical-outstanding external-IMEM adapter, not the configured/generated SmallBoom Frontend. Gate 5.1 added fetch ID, 32-bit epoch, and expected-address matching, stale-response drain, architectural redirect ownership, fetch-fault propagation, non-RVC target-alignment faults, and a one-entry lane-0 Decode handoff. It still lacks RVC, Fetch Buffer, FTQ, dynamic prediction/RAS, and ICache.

The repository-generated SmallBoom target proves fetch width 4, decode width 1, 8 fetch bytes, 8 Fetch Buffer entries, 16 FTQ entries, 8 branch tags, RVC, a 32-entry RAS, composed loop/TAGE/BTB/micro-BTB/BIM prediction, and a 16 KiB four-way ICache. Exact source-level policy for several predictor structures remains UNKNOWN and is not required by Gate 5.1.

Gate 5.1R completes final acceptance. A non-product verification wrapper exposes the mandatory redirect/ownership/fault/hold/runtime-reset state without duplicating Frontend next-state logic, and generated RTL passes 33/33. The canonical next-state repair removes the native two-call request bubble and restores W3 to 400/400; S0 with no scheduling directive is accepted, while S1/S3 remain rejected. See `reports/gate5_1_frontend/gate5_1_results.md`.
