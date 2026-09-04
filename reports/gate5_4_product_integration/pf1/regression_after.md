# PF1 Regression After

| Suite | Result |
|---|---:|
| PF1 directed | 1817/1817 PASS |
| PF1 random | 25,165,824 checks, zero errors |
| PF1 native/CSim/full-core RTL programs | 8/8 each PASS |
| PF1 focused generated RTL | 64/64 PASS |
| W3 software and trace preservation | 400/400 PASS |
| W3 full-program architectural diff | 10/10 PASS |
| W3 partial-order | 7/7 PASS |
| RV64M directed/random/programs | PASS / PASS / 15/15 |
| RVC full-core native | 11/11 PASS |
| Fetch Buffer focused | 169/169 PASS |
| W4 multi-writeback | 13/13 PASS |
| Canonical csynth | 8/8 PASS |

The Gate 5.3 Fetch Buffer persistent-random test has an accepted-baseline gap:
both PF1 and a baseline-equivalent build report exactly `drop=425388`,
`ordering_error=174170`, and zero duplicate/stale-side-effect/post-flush
errors. This result is excluded from the PF1 regression delta but remains open.
