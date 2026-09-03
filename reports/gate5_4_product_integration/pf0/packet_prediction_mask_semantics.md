# Packet Prediction Mask Semantics

```text
PRODUCT_PACKET_PREDICTION_MASK_POLICY=POST_PREDICTION_EFFECTIVE_MASK_FROZEN_BEFORE_ATOMIC_ADMISSION
```

F1 `packet_valid_mask` and initial `live_lane_mask` store the final admitted mask, not the pre-prediction builder mask. If lane 0 is predicted taken, effective mask is `01`; lane 1 never becomes an FB token, uop, ROB entry, or live FTQ lane. This avoids enqueue-then-flush behavior.

For a lane-0 conditional predicted not taken, mask `11` is admitted. Lane 1 carries the branch dependency mask and the same FTQ owner identity with lane 1. If actual outcome is taken, branch recovery kills lane 1 and every other younger token, and F1 clears lane 1 exactly once. If actual is not taken, both lanes retire normally.

| Case | Initial F1 live mask | Resolution change |
|---|---|---|
| A: lane0 predicted taken | `01` | lane1 never exists |
| B: lane0 predicted NT, actual NT | `11` | each lane clears at commit |
| C: lane0 predicted NT, actual taken | `11` | lane1 clears by squash; lane0 later commits |
| D: owner packet killed by older branch | `01` or `11` | every still-live admitted lane clears once |

Packet payload discarded before admission must never be resurrected after a later correction; recovery refetches from the correct PC.
