# M3C Regression Before

Frozen M3B handoff reports the following accepted results:

- M1 decode: 42/42 PASS.
- M2A directed: 51/51 PASS; arithmetic random: 131072 vectors PASS.
- M2B integration: 30/30 PASS; persistent random and multiply full-core PASS.
- M3A directed: 104/104 PASS; arithmetic random 65536 and protocol random 65536 cycles PASS.
- M3B directed: 167/167 PASS; integration random 128 x 1024 PASS.
- M3B native/csim/generated-RTL full-core programs: 10/10 each PASS.
- M3B focused RTL: 26/26 PASS.
- W3 software: 400/400 PASS; W3 focused RTL: 11/11 PASS.
- W4 directed: 95/95 PASS; W4 persistent random: 128/128 PASS; W4 focused RTL: 20/20 PASS.
- Reset architecture: 14/14 PASS.
- Gate 3.9 XSim: 49/49 PASS.
- Frozen C++/csim traces, full-program diff 10/10, and partial-order checks PASS.
- M3B canonical csynth: 8/8 complete, both full-core periods 6.341 ns, 3 DSP, 16 BRAM.

These are entry baselines, not substitutes for the required M3C reruns.
