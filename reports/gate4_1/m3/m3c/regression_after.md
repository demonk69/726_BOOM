# Gate 4.1 M3C Regression After

Result: **PASS**.

- M3C directed: 1458 checks, 0 failures.
- M3C random: 256 fixed seeds x 2048 cycles, all 13 RV64M operations covered, zero mismatch/drop/duplicate/stale/starvation failures.
- Native and Vitis csim full-core programs: 15/15 each.
- M3C focused generated RTL: 30/30; M3C full-core generated RTL: 15/15.
- M2 preserved RTL: 10/10 focused and 2/2 full-core; M3B preserved RTL: 26/26 focused.
- M3B directed/random recheck: 167/167 and 128 x 1024 PASS.
- W3 canonical software: 400/400; W3 focused RTL: 11/11.
- W4 directed/random: 95/95 and 128/128; W4 focused RTL: 20/20.
- Gate 3.9 generated RTL: 49/49.
- C++/csim normalized traces: 7/7; architecture/event/cycle checks: 21/21; full-program diff: 10/10; partial order: 7/7.
- Eight canonical csynth targets: 8/8; both full-core periods 6.341 ns, 3 DSP, 16 BRAM, and `PipelineType=no`.
