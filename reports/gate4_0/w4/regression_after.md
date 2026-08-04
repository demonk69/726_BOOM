# Gate 4.0 W4E Software Regression After

Result: **PASS** for W4E software/random integration on the approved W4D product.

- W4E independent persistent random: 128/128 fixed seeds, 16384/16384 randomized cycles, plus 128/128 production-order collision probes.
- Eligible write-port arbitration wait only: observed 1 cycles, bound 1 = ceil(3 fixed source holds / 2 writers) - 1. This is not a general completion-liveness claim; 487 fence-blocked cycles are classified separately.
- Publication peaks: 3 sources, 2 PRF writes, 3 wakeups, 3 bypasses.
- Token conservation: 21728 offered = 9740 committed + 10803 killed + 1185 faulted; source-event conservation: 24504 = 24504.
- Accepted-token conservation: 20717 = 9740 committed + 10310 killed + 667 faulted + 0 pending.
- Integrity: 0 dropped, 0 duplicated, 0 stale side effects, 0 unexplained.
- Reset/branch recovery: 1311 in-flight resets, 1233 delayed stale post-reset responses, 158 branch-killed tokens.
- Full load `DmemRequest` payload checks: 1446.
- Cumulative W4A/W4B/W4C/W4D directed suites: 95/95 PASS, derived from parsed suite records.
- W3 canonical software preservation: 400/400 PASS; includes W2/W3 random and reset suites.
- C++/csim normalized traces: 7/7 PASS; normalized architecture/event/cycle checks: 21/21 PASS.
- Full-program architectural diff: 10/10 PASS; partial order: 7/7 PASS.
- Merged generation, merged compile, synth-top compile, and core-top compile: 4/4 PASS.

Observed concurrency and conservation fields are in `concurrency_metrics.csv`. Only genuinely measured stage latencies are reported, in `regression/w4e/latency_metrics.csv`; unlike W3/W4E event subtraction is intentionally absent.

Final follow-on gates pass: focused W4 RTL 20/20, current W3 RTL 11/11, full-core XSim/normalization/architecture 49/49, and source-bound final csynth 7/7. The generated two-bank LVT PRF has independent `bank0_we0` and `bank1_we0`; the write stage reaches final II=1 without a dependence waiver.
