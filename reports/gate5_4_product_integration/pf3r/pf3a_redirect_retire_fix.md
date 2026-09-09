# PF3A Redirect/Retire Retry Fix

`pf3_generation_reuse` originally stalled after 71 commits with FTQ occupancy
32. A taken JAL caused `FtqFoundation::step()` to prioritize redirect and report
the simultaneous retire input as rejected. `frontend_mode_module<true>()` then
cleared both pending inputs, permanently leaving one live owner lane at the FTQ
head. Younger entries could commit but could not reclaim past that owner.

The fix in `src/frontend.cpp` retains a retire event rejected specifically by a
redirect step and presents it on the next step. Exception-owner deferred retire
remains ordered behind an already retained regular retire. A rejected redirect
still discards its invalid exception-deferred event.

Post-fix native evidence for `pf3_generation_reuse` is 84 allocations, 84
retires, 84 reclaims, two wraps, 52 index-generation reuses, one redirect, and
completion in 259 core steps. Current canonical RTL also completes with visible
redirect/retire retry cycles, two wraps, and 59 post-depth allocations.

The fix does not alter FTQ entry width, depth, reference layout, generation
width, storage, reset, stale-reference, turnover, or allocation policy.

Focused PF2 RTL subsequently exposed stale prediction-result fields on a JALR
packet following a conditional prediction. Clearing those observability fields
when each new packet is built, rather than when the prior packet is accepted,
preserves the accepted conditional result for its admission cycle and prevents
non-conditional packets from reporting old metadata. The final PF2 focused RTL
run passes all 116 cases.
