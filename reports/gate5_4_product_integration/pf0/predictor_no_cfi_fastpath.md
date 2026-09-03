# Predictor No-CFI Fast Path

```text
PRODUCT_NO_CFI_PREDICTOR_POLICY=CFI_ONLY_REQUEST_WITH_NO_CFI_BYPASS
```

`EVERY_PACKET_PREDICTOR_REQUEST` wastes one stateful transaction to receive `prediction_valid=false`, blocks the single frontend path, and lowers straight-line throughput. `CFI_ONLY_PREDICTOR_REQUEST` uses P1's `CFI_NONE` result to admit the unchanged packet without P2. It is simpler, preserves P2 bandwidth, and does not weaken FTQ uniformity because the FTQ entry explicitly records `prediction_valid=false` and `CFI_NONE`.

Fault-only and illegal packets use the same no-request path, while retaining their fault payload through FB/uop/ROB.

## JAL Fast Path

```text
PRODUCT_JAL_PREDICTION_POLICY=STATIC_PREDECODE_TAKEN_BYPASS_WITH_UNIFORM_FTQ_METADATA
```

P1 already computes the exact JAL target and P2 adds no information. Product logic should mask younger lanes and select the static target without a P2 wait, while populating the same FTQ prediction fields as P2 would. This preserves verification and future extension boundaries without imposing stateful latency.
