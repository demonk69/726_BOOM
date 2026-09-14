# PF5 Stage Delta Attribution

PF5 adds 2,522 LUT and 786 FF to `boom_core_top`; BRAM, DSP, and estimated
period are unchanged. The delta is attributable to ROB resolved-outcome state,
FTQ/predictor identity qualification, and the one-entry pending update handoff.
The canonical predictor remains the sole BIM owner and still synthesizes as
LUTRAM. No alternate training table was introduced.

`PPA_REVIEW_REQUIRED=false`

`GATE5_4_PF5_PPA_BLOCKER=false`
