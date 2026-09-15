# LSU, DCache, and MMU Boundary

LSU owns instruction ordering, LQ/SQ allocation and age, dependencies, forwarding, violation detection, replay initiation, memory-operation identity, and commit-store eligibility.

DCache owns tags/data, hit/miss, refill/writeback, MSHRs, replacement, and any future coherence-facing interface.

MMU owns VA-to-PA translation, TLB, PTW, permissions, page faults, and the translation-side PMP boundary. The current project has address-width/cache constants but no product DCache/MMU behavior in the LSU; Gate6 must keep these absent rather than infer implementation from constants.

`G6_LSU_DCACHE_BOUNDARY_FROZEN=true`

`G6_LSU_MMU_BOUNDARY_FROZEN=true`
