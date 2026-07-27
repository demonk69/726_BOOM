# D4_IQ_COMPACTION Change

Single-variable structural change: make IQ branch-kill compaction survivor-based.

Changed files:

| File | Purpose |
|---|---|
| `src/branch.cpp` | Replaces kill-then-compact with one stable survivor pass for branch recovery IQ cleanup. |
| `src/issue.cpp` | Folds pre-dispatch invalid/granted removal, branch kill, resolved-mask clear, and busy refresh into one stable survivor pass. |
| `tb/differential/iq_compaction_tests.cpp` | Adds directed IQ compaction tests for kill none/all/alternating, issue+kill priority, dispatch+issue+kill, full IQ, wrap compact, nested masks, and oldest-ready preservation. |

Architecture and cycle intent:

| Constraint | Status |
|---|---|
| IQ depth unchanged | Preserved |
| Stable survivor order | Preserved |
| Oldest-ready selection | Intended preserved; tested |
| Dispatch position after pre-dispatch compact | Preserved |
| Same-cycle issue/kill/dispatch priority | Intended preserved; tested |
| Multi-cycle compaction | Not introduced |
| CORE_CYCLE pipeline | Disabled |

This variant is independent from D4 local kill bitmap and starts from the accepted baseline source.
