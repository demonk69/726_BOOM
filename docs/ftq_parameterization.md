# FTQ Parameterization

## Recommended Parameters

```text
FTQ_ENTRY_GRANULARITY=PER_FETCH_PACKET
FTQ_DEPTH=32
FTQ_IDX_BITS=5
FTQ_ALLOCATION_POINT=FETCH_BUFFER_ENQUEUE
FTQ_RECLAIM_POLICY=PER_FTQ_LIVE_UOP_COUNT
CURRENT_MINIMAL_FTQ_ENTRY_BITS=69
FUTURE_PREDICTOR_FTQ_ENTRY_BITS_ESTIMATE=288
```

Current-minimal entry bits are `base_pc[63:0] + lane_valid_mask[1:0] + live_count[1:0] + valid`. The future estimate adds `next_pc[63:0]`, predicted target 64, direction/valid 2, CFI slot/type 4, GHR snapshot 32, predictor response metadata 16, RAS pointer 5, and generation 32. Future widths are placeholders pending predictor-interface review, not approved product fields.

Static state uses:

```text
total_state_bits = depth * entry_bits
                 + 2 * ceil(log2(depth))
                 + ceil(log2(depth + 1))
```

| Depth | Current 69-bit entry | Future 288-bit estimate |
|---:|---:|---:|
| 8 | 562 | 2314 |
| 16 | 1117 | 4621 |
| 32 | 2224 | 9232 |

These are logical bits, not LUT/FF/BRAM predictions. Register, LUTRAM, BRAM, and AUTO must be synthesized as standalone candidates. Narrow valid/count/pointers should remain registers; PC and future predictor metadata should be separable arrays so one wide field does not force an unsuitable mapping. No storage directive is approved by F0.

## F1 Selected Parameters

```text
F1_CANONICAL_FTQ_DEPTH=32
FTQ_INDEX_BITS=5
FTQ_GENERATION_BITS=32
PREDICTOR_GENERATION_BITS=32
F1_FTQ_ENTRY_BITS=211
F1_TOTAL_STATE_BITS=6800
F1_LIVE_TRACKING_POLICY=LANE_MASK
F1_CANONICAL_FTQ_STORAGE=LUTRAM
F1_CANONICAL_FTQ_RESET_POLICY=CONTROL_ONLY
```

The 211-bit entry is derived from the now-frozen P2 foundation rather than the
old 288-bit pre-review estimate. It stores the P2 predictor generation
independently from the FTQ allocation generation; the transient P2 request
token is not retained. At depth 32, entry storage is 6752 bits and
ring/generation control is 48 bits. Native behavior is parameterized at depths
8/16/32/64; depths 2/4 are also instantiated for bounded model exploration.

Vitis depth-32 experiments report AUTO at 3040 LUT/974 FF/6 BRAM/4.348 ns,
LUTRAM at 3006 LUT/1358 FF/0 BRAM/3.788 ns, and BRAM at 2895 LUT/924 FF/15
BRAM/4.907 ns. LUTRAM is selected to avoid disproportionate BRAM use and has
the best estimated period. Full payload reset adds 220 LUT over CONTROL_ONLY
under the AUTO comparison without improving the timing estimate.
