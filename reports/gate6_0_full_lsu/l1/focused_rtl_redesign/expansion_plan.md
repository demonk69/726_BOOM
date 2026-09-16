# L1R Focused RTL Expansion Plan

Expansion is not part of L1R2A and has not started.

## Image Families

| Family | Cases | Planned implementation |
|---|---:|---|
| A queue allocation/reuse | 0-12, 41-44 | Stateless LQ/SQ accept, finish, reclaim, wrap, generation, and count images |
| B response identity | 13, 16-25, 45, 47 and response variants | Stateless pending/ROB/LQ tuple setup followed by canonical response classification/finish |
| C SQ identity | 14-15, 42, 44, 46 and store variants | Stateless SQ match/reclaim image derived from pilot image 5 |
| D blocking and backpressure | 26-29, 48 | Exact older-store predicate for focused policy; canonical full-core PATH_B for channel and commit ordering |
| E branch recovery | 30-35 and squash variants | Stateless `branch_complete_event` image derived from pilot image 4 |
| F flush/reset | 36-40, 49 and flush variants | Stateless fixed global flush where small; canonical full-core PATH_B for exception and staged reset |

## Execution Policy

1. Generate images only from the complete `focused_rtl_case_partition.csv`; never infer coverage from the old 17-case aggregate.
2. Execute serially under 900 s, 4 GiB soft, and 8 GiB hard limits. Stop expansion on the first new family-level resource violation.
3. Keep each wrapper stateless and expose raw post-state. Add expected checks only to SV/host oracles.
4. Reuse one generated image for parameter variants only when the sequence and product boundary are identical and values remain RTL inputs.
5. Use current-source `boom_core_top` PATH_B for reset, exception/commit, dmem backpressure, and composite issue behavior that cannot fit PATH_A.
6. Require one ordered PASS row per catalog ID before changing the Gate count from the retained aggregate `17/80` to `80/80`.

## Stop Conditions

- Any product source hash change invalidates this partition and requires a new candidate declaration.
- Any use of static mutable core state, command opcodes, cross-call state, or copied product logic invalidates the image.
- A timeout, hard resource stop, missing RTL, missing oracle sentinel, or ambiguous case-to-image mapping is FAIL/BLOCKED, never PASS.
