# FTQ Live Tracking

`F1_LIVE_TRACKING_POLICY=LANE_MASK`.

Count-only state cannot distinguish a valid second retirement from a repeated
retirement of the same lane. The two-bit live mask initializes to `01` or
`11`; live-uop count is its popcount. Retire and squash use the minimal safe
identity `{ftq_idx,generation,lane}`. Clearing an absent, already-cleared,
invalid, or stale lane is rejected and cannot underflow or reclaim storage.
