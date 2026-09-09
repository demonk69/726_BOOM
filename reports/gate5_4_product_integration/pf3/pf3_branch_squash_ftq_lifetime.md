# PF3 Branch Squash FTQ Lifetime

PF3 connects only the existing actual backend branch recovery. A validated
branch owner causes an F1 redirect retaining that owner, intersecting its live
mask with lanes through the owner lane, and invalidating every younger FTQ
entry. Thus lane-0 recovery in a mask-11 packet kills lane 1 without releasing
lane 0. No FTQ prediction metadata is read for comparison and no conditional
prediction steering is enabled.
