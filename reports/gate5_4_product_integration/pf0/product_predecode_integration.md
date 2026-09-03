# Product Predecode Integration

```text
PRODUCT_PREDECODE_POINT=AFTER_CANONICAL_FETCH_PACKET_BUILD_BEFORE_PREDICTOR_REQUEST_OR_FETCH_BUFFER_ADMISSION
```

The point is immediately after `build_fetch_packet` has produced complete canonical instructions and before `pending_packet` becomes enqueue-visible. P1 must never inspect a partial 16-bit parcel.

| Case | Required treatment |
|---|---|
| response lower parcel | Predecode only after length classification/decompression or complete 32-bit assembly |
| response upper parcel | Predecode RVC immediately; retain a 32-bit start as carry without predecode |
| RVC | Predecode decompressed canonical instruction, retaining length 2 and original PC |
| cross-word completion | Predecode only after carry plus next lower parcel form the full instruction |
| carry + upper RVC | Predecode completed lane 0 and canonical RVC lane 1 in PC order |
| packet `01`/`11` | Run P1 on valid, nonfaulting lanes; choose earliest CFI |
| fetch fault/illegal RVC | `predecode_valid=false`; do not send a predictor request |
| no complete instruction | Keep carry; no packet, predecode, predictor request, FB enqueue, or FTQ allocation |

For a lane-0 selected CFI, fallthrough is `cfi_pc + instruction_length`; it must not blindly use the builder's `next_pc`, which may include a lane later removed by prediction.

P1's generic helper that removes lanes younger than any selected CFI is not itself the product final-mask policy. Selection and masking remain separate: mask lane 1 for a predicted-taken conditional or static JAL, but retain lane 1 for predicted-not-taken conditional and foundation JALR with no target prediction.
