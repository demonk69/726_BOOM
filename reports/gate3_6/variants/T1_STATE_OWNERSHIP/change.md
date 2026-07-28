# T1_STATE_OWNERSHIP

Status: `NOT_RUN_NO_STRUCTURAL_DEFECT`.

The accepted source already has exactly one persistent `BoomCoreState` in each selected top. `boom_core_step` and all major helpers receive state by reference; no active `BoomCoreState next_state` copy exists. T4 retains this ownership and removes 37684 LUT by changing only reset elaboration, so changing ownership would add risk without addressing the measured cause.

No source change was applied.
