# Accepted Stage Manifest

| Stage | Commit | Accepted status | Product status |
|---|---|---|---|
| Gate 5.3 Fetch Buffer | `ef051ca4e3673d663f9d10b55029c956fbc0052a` | `GATE5_3_FETCH_BUFFER_VERIFIED=true` | Integrated and frozen |
| Gate 5.4 P1 CFI Predecode | `fb1f34719dd09332e54cbc0766dd68df81bd545a` | `GATE5_4_P1_CFI_PREDECODE_VERIFIED=true` | Standalone only |
| Gate 5.4 P2 Predictor Foundation | `5bc5644b1b47697a439ce1605b87068b76eee68d` | `GATE5_4_P2_PREDICTOR_FOUNDATION_VERIFIED=true` | Standalone only |
| Repository Hygiene R1 | `7b6a7a66bf190f8d8faa990f3f88bcb99de99c8a` | Accepted | Policy preserved |
| Gate 5.4 F1 Standalone FTQ | `a905d65d00f3c27f6bb2f1cfda7bbfc83c72edac` | `GATE5_4_F1_STANDALONE_FTQ_VERIFIED=true` | Standalone only |

F1 freezes depth 32, LUTRAM, CONTROL_ONLY reset, 211-bit entries, 32-bit allocation generation, lane-mask live tracking, and stale identity `{ftq_idx,generation,lane}`. The F0 288-bit estimate is not an implementation value.

P2 freezes 256 2-bit BIM entries, LUTRAM, lazy-valid initialization, `(pc >> 1) & 255`, update-forward-new-value, commit-qualified conditional training, and logical request N/response N+1 blocking behavior. Standalone HLS latency 3 and II 4 are not product-cycle values.

Accepted standalone evidence remains: F1 directed/exhaustive 275944648 checks with zero failures, random 587202560 checks with zero errors, P1/P2/F1 composition 11080 checks with zero failures, RTL 193/193 PASS, and csynth 7/7 PASS. Canonical F1 PPA is 3006 LUT, 1358 FF, 0 BRAM, 0 DSP, 3.788 ns.
