# P0 Contract Snapshot

- Request point: `PARCEL_PREDECODE_STAGE`
- Predecode required: true
- First CFI: earliest valid CFI in PC order
- Foundation: static target predecode plus BIM
- Predictor latency: fixed one-step blocking packet
- BTB, RAS, and global history: not required and not implemented
- FTQ retains prediction validity, direction, optional target, CFI lane/type,
  BIM update identity, and generation through future resolution/update.
- Product predictor and FTQ integration remain false.
