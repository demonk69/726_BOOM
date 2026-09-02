# FTQ Redirect Semantics

A valid redirect owner keeps all older entries and the owner entry, removes
the contiguous younger suffix, restores tail immediately after the owner, and
intersects owner live lanes with `surviving_lane_mask`. Thus a lane-0 branch
can kill lane 1 without deleting its packet; a lane-1 branch retains both live
lanes unless another event has already retired one. Redirect is exclusive and
has priority over allocation and explicit retire/squash inputs.

The same abstract retain-owner/kill-younger operation models exception capture:
the fault owner remains until EPC/cause capture, after which younger entries
are removed. F1 does not modify or claim closure of product exception handling.
