# Minimal Predictor Response Contract

The contract is logical documentation only. It does not add fields to product structs.

| Field | Classification | Consumer and reason |
|---|---|---|
| `prediction_valid` | REQUIRED_FOR_FOUNDATION | distinguishes an actual prediction from sequential fallback |
| `taken` | REQUIRED_FOR_FOUNDATION | selects static target versus fall-through and enables direction comparison |
| `target_valid` | REQUIRED_FOR_FOUNDATION | prevents taken without a usable next PC; false causes sequential/no-prediction behavior |
| `target` | REQUIRED_FOR_FOUNDATION | Frontend next-PC selection and later target comparison |
| `cfi_lane` | REQUIRED_FOR_FTQ | ties response to earliest packet CFI and supports same-packet recovery |
| `cfi_type` | REQUIRED_FOR_UPDATE | separates direction, target, slot, and return outcomes |
| `metadata_token` | REQUIRED_FOR_UPDATE | opaque token identifying the BIM entry/version needed at commit training |
| `generation` | REQUIRED_FOR_FOUNDATION | rejects delayed response after redirect/reset; foundation reuses Frontend epoch |

The request also carries exact CFI PC, CFI type, static target validity/target, packet epoch, and a pending request token. The response must echo the token or otherwise be accepted only against the sole live pending request.

For the selected foundation, conditional branches use BIM direction plus predecoded static target. JAL is always taken using its static target without needing a BIM counter. JALR/return has no valid foundation prediction. A packet with no valid CFI issues no request and bypasses prediction without the one-cycle wait; fault delivery is unchanged. Confidence, provider identity, alternate prediction, GHR/RAS snapshots, and BTB way/tag are `DEFER` because there is no selected consumer.
