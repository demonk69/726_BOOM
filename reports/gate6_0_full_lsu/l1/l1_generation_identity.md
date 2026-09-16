# L1 Generation And Identity

Every LQ and SQ physical slot owns a 16-bit generation. Allocation increments modulo 65536. Ordinary free, branch squash, global flush, and precise exception flush preserve generation; staged reset establishes zero. LQ delayed mutation validates `{lq_index,generation,rob_idx,rob_allocation_id,transaction_id}`. SQ commit reclaim validates `{sq_index,generation,rob_idx,rob_allocation_id}`.

Directed seeds covered `0xfffe -> 0xffff -> 0x0000`. An old generation was rejected after slot reuse for both LQ response and SQ reclaim paths.

Finite lifetime assumption: no delayed event may survive 65536 allocations of the same slot with all other identity fields also reused. The preserved 32-bit transaction ID and ROB allocation ID provide additional independent identity domains.
