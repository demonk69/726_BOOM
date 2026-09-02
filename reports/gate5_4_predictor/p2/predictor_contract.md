# Standalone Predictor Contract

## Request And Response

`PredictorRequest` carries exact CFI PC, selected lane/type, optional static target, generation, and request token. `PredictorResponse` returns prediction validity, direction, optional target, copied lane/type/generation/request token, and the BIM metadata index.

- Conditional: prediction is valid; direction is BIM counter MSB; target is valid only when taken and a static target was supplied.
- JAL: prediction is valid and taken; target validity follows the static target input.
- JALR and no CFI: `prediction_valid=false`, `taken=false`, and `target_valid=false`.
- Invalid BIM entries behave as counter `01`, weak not-taken.

## Update

Training occurs only when `valid && commit_qualified`, CFI type is conditional, update generation equals active generation, and metadata token equals the index derived independently from update PC. Counters saturate over `00,01,10,11`. Same-index lookup/update collision policy is `UPDATE_FORWARD_NEW_VALUE`.

## Scope

The contract is standalone. There is no product Frontend request, FTQ metadata lifetime, Execute correction, or Commit update connection in P2.
