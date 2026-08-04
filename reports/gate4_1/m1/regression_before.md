# Gate 4.1 M1 Regression Baseline

Frozen predecessor result: **PASS** at Gate 4.0 W4 commit `c3da959`.

- W3 canonical software preservation: 400/400 PASS.
- W4 cumulative directed checks: 95/95 PASS.
- Reset architecture: 14/14 PASS.
- W4 persistent random differential: 128/128 seeds and 16,384 cycles PASS.
- C++/Vitis csim normalized traces: PASS and frozen.
- Full-program architectural diff: 10/10 PASS.
- Partial-order checks: 7/7 PASS in the W4 product regression.
- Final source-bound csynth: 7/7 targets PASS.
- `boom_core_top`: 111869 LUT, 25094 FF, 16 BRAM_18K, 3 DSP, 6.025 ns.
- `CORE_CYCLE`: `Pipelined=no`.

M1 may add exact decode legality and issue classification only. It must not update frozen trace expectations or claim executable RV64M arithmetic.
