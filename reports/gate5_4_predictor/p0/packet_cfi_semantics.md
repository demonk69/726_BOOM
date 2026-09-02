# Two-Lane Packet CFI Semantics

`FIRST_CFI_POLICY=EARLIEST_VALID_CFI_IN_PC_ORDER`

Only the earliest valid CFI in packet program order may control the next fetch PC. Predecode may describe both lanes internally, but one selected CFI and one prediction response belong to the packet/FTQ entry.

| Scenario | Selected CFI | Effective mask if selected CFI predicted taken | Result |
|---|---|---:|---|
| lane0 normal + lane1 conditional | lane1 | `3` | Both lanes remain; no younger same-packet instruction exists |
| lane0 branch + lane1 normal | lane0 | `1` | lane1 is prediction-killed before Fetch Buffer enqueue |
| lane0 JAL + lane1 normal | lane0 | `1` | static unconditional redirect; lane1 killed |
| lane0 JALR + lane1 normal | lane0 | `3` in foundation | no dynamic target prediction, so sequential delivery continues until Execute |
| lane0 branch + lane1 branch | lane0 | `1` when lane0 predicted taken; otherwise `3` | lane1 can execute only down lane0 fall-through |
| lane0 RVC CFI + lane1 normal | lane0 | `1` if predicted taken | identical ordering; length is 2 |
| lane0 normal + lane1 RVC CFI | lane1 | `3` | lane1 controls next fetch after both accepted slots |
| lane0 predicted taken | lane0 | `1` | lane1 never enters Fetch Buffer and does not increment live count |

The effective packet mask is finalized before atomic Fetch Buffer enqueue/FTQ allocation. The FTQ records that post-prediction mask, not the parser's original mask. Therefore a prediction-killed lane is not a live uop and needs no later decrement.

Conceptually it remains a discarded younger lane from the same prospective packet, but it is not part of the allocated FTQ entry because allocation occurs after prediction. If a later Execute correction discovers a lane0 prediction was wrong, recovery redirects to fall-through and refetches lane1; it does not resurrect discarded payload.

When lane0 is predicted not taken, lane1 remains in the same FTQ entry. If lane0 later resolves taken, recovery decrements/kills lane1 as a younger same-entry uop and retains the owner entry until all remaining live/update obligations finish.
