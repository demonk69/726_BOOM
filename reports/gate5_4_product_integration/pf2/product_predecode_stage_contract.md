# Product Predecode Stage Contract

P1 runs only after `build_fetch_packet` has produced complete canonical instructions. Valid nonfaulting lanes are passed to the canonical `predecode_cfi_packet`; PF2 contains no second decoder and never decodes immediates locally.

| Input form | Contract | Evidence |
|---|---|---|
| RVC | Decompress first, preserve `is_rvc` and original PC | directed/random and full-core RVC |
| aligned 32-bit | Complete word passed once | directed base branch/JAL |
| cross-word | Carry is retained; predecode starts only after completion | directed cross-word case |
| mixed width | Complete lanes remain in PC order | random/full-core RVC |
| mask01 | Only lane 0 considered | base instruction tests |
| mask11 | Earliest valid CFI selected | lane0/lane1 tests |
| fetch fault | The oldest fault survives and removes younger lanes; P1 examines only older nonfaulting lanes | directed fault precedence |
| older JAL, younger fault | The older JAL is the oldest surviving architectural token, steers Frontend, and masks the younger fault | directed and focused RTL precedence cases |

`PRODUCT_PREDECODE_POINT=AFTER_CANONICAL_FETCH_PACKET_BUILD_BEFORE_PREDICTOR_REQUEST_OR_FETCH_BUFFER_ADMISSION`

`PF2_PACKET_PRECEDENCE=OLDEST_SURVIVING_ARCHITECTURAL_TOKEN_IN_PC_ORDER`
