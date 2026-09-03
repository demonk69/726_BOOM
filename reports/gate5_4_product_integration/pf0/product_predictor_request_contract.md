# Product Predictor Request Contract

```text
PRODUCT_PREDICTOR_REQUEST_GRANULARITY=PER_PACKET_SELECTED_EARLIEST_CONDITIONAL_CFI
```

P1 examines up to two complete lanes but selects only `EARLIEST_VALID_CFI_IN_PC_ORDER`. A packet never issues two P2 requests. The product sends P2 one request only when the selected CFI is conditional. Request identity binds packet context, selected lane/type/PC, static target/fallthrough, Frontend epoch, and a pending token.

No-CFI and fault-only packets bypass P2. JAL is handled by static predecode. JALR has no target prediction in the foundation and is recorded as no-target prediction without spending a stateful request.
