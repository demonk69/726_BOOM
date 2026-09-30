# PF6 Progress Probe

## Classification

`PF6_PROGRESS_TRIAGE=PASS`

`PF6_PROGRESS_FAILURE_CLASS=HARNESS_WAIT_CONDITION_BUG`

XSim simulation time advanced normally. The 1, 10, and 100 physical-cycle
probes completed in 5.17, 5.14, and 5.23 seconds including XSim startup. A
10,000-cycle probe reached 100 us and observed reset completion. There was no
delta-cycle oscillation, iteration-limit warning, or fixed-timestamp stall.

The previous 1000-cycle diagnostic did reach all 1000 post-reset testbench
cycles after the harness repair. It observed one IMEM request and response but
was intentionally too short to complete a case; the historical minimum case
requires about 22,741 cycles.

Using the 1000 physical-clock control probes including startup:

- PF6: 1000 / 5.15 = 194.17 cycles/s.
- RVC: 1000 / 5.38 = 185.87 cycles/s.
- RVC/PF6 slowdown ratio: 0.96x; PF6 is not slower.

The repaired smoke completed in 22,742 cycles with 10 commits, then the full
suite completed 12/12 programs and 2/2 mandatory fault checks.
