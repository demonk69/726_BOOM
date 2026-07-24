# Gate 3.1A Partial-Order Diff

Legacy BOOM-vs-HLS global event-order failures are classified as `VALIDATION_METHOD_FALSE_POSITIVE` for these traces.

Legal reorder events: 8
Real partial-order violations: 0

| Program | Status | Matched uops | Legal reorders | Real violations | Insufficient identity | Insufficient signal |
|---|---|---:|---:|---:|---:|---:|
| independent_alu | LEGAL_REORDER | 8 | 1 | 0 | 0 | 47 |
| raw_chain | LEGAL_REORDER | 8 | 1 | 0 | 0 | 47 |
| branch_taken | LEGAL_REORDER | 9 | 2 | 0 | 0 | 52 |
| branch_not_taken | LEGAL_REORDER | 10 | 2 | 0 | 0 | 58 |
| nested_branch | LEGAL_REORDER | 10 | 2 | 0 | 0 | 59 |

## Conclusions

- Commit order matches for all five prefix programs.
- Branch resolve before older commit is observed and is legal out-of-order behavior, not a same-uop order violation.
- RAW/WAR/WAW timing cannot be fully closed from these traces because issue, wakeup, rename-source, and complete events are not exposed.
- No real partial-order functional violation is detected in the available signal set.
