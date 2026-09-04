# Predictor Response Stale Policy

`PF2_PREDICTOR_RESPONSE_STALE_POLICY=MATCH_FRONTEND_EPOCH_PLUS_PREDICTOR_GENERATION_PLUS_REQUEST_TOKEN_PLUS_CFI_LANE_TYPE_ELSE_DRAIN_NO_SIDE_EFFECT`

Runtime reset, architectural exception redirect, branch recovery redirect, and generic flush invalidate pending packet context before prediction use. A response is actionable only if epoch, P2 generation, request token, selected lane, and conditional type match. Otherwise it is consumed/drained without PC, packet, carry, Fetch Buffer, or predictor table side effects. Exception and branch recovery dominate safe JAL steering and normal sequential fetch.
