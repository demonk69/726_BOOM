# Frontend Architecture Review

Gate 5.0 F0 establishes that the canonical HLS Frontend is a minimal one-instruction, one-logical-outstanding external-IMEM adapter, not the configured/generated SmallBoom Frontend. It has a PC, request ID matching, one buffered response, branch recovery, and lane-0 decode handoff. It lacks epoch-safe stale rejection, propagated fetch faults, RVC, real fetch packets, Fetch Buffer, FTQ, dynamic prediction/RAS, ICache, and architectural exception/return redirect targets.

The repository-generated SmallBoom target proves fetch width 4, decode width 1, 8 fetch bytes, 8 Fetch Buffer entries, 16 FTQ entries, 8 branch tags, RVC, a 32-entry RAS, composed loop/TAGE/BTB/micro-BTB/BIM prediction, and a 16 KiB four-way ICache. Exact source-level policy for several predictor structures remains UNKNOWN and is not required by Gate 5.1.

The minimum safe increment is protocol foundation only. It must close the reset-ID stale alias, define redirect arbitration and epoch semantics, propagate IMEM faults, and preserve one-wide decode/back-end behavior. Detailed evidence is in `reports/gate5_0/f0/current_frontend_behavior.md`, `target_parameter_evidence.csv`, and `frontend_gap_matrix.csv`.
