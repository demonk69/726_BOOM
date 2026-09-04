# PF1 Throughput Analysis

PF1 does not change dispatch, issue, execute, writeback, or commit width.
Normal execution retains the accepted one-wide commit topology and
unpipelined `CORE_CYCLE`. Exception take is a serializing event: no younger
instruction commits, speculative queues are cleared, and Fetch restarts at the
trap vector with a new epoch. Recovery latency is intentionally outside the
normal steady-state throughput claim.

W3 preservation reports 400/400 PASS, including 10/10 full-program
architectural diff and 7/7 partial-order checks. RV64M native programs remain
15/15 PASS with maximum observed divider latency 66 cycles.
