# PF3 Prediction Metadata Contract

Each accepted packet stores the canonical F1 211-bit entry fields: packet base
PC, final packet mask/live mask, prediction-valid/taken, target-valid/target,
selected CFI lane/type, BIM metadata index, independent P2 predictor generation,
and F1 allocation generation. P2 request tokens are consumed by the Frontend
matcher and are not FTQ update metadata.

JAL static metadata is retained with the final mask. Conditional metadata is
retained but remains `SHADOW_ONLY` and cannot steer PC. JALR stores
`prediction_valid=false` and no fabricated target.
