# M3B Regression After

- M1 decode: 42/42 PASS.
- M2A and M2B multiply regression: PASS, including M2B directed 30/30 and full-core checks.
- M3A Divider: directed 104/104, arithmetic random 65536, protocol random 65536 cycles, zero mismatch.
- M3B directed integration: 167/167 checks PASS.
- M3B persistent random: 128/128 seeds x 1024 cycles PASS; all drop, duplicate, stale-side-effect, instability, and value-mismatch counters are zero.
- Native full-core DIV programs: 10/10 PASS with commit and writeback checks.
- Vitis csim full-core DIV programs: 10/10 PASS.
- Focused generated RTL: 26/26 PASS, including five Divider reset extensions.
- Generated `boom_core_top` DIV programs: 10/10 PASS.
- Gate 3.9 generated RTL preservation: 49/49 PASS.
- W4 directed: 95/95 PASS; W4 persistent random: 128/128 PASS.
- W3/W2/branch/reset/LSU/IQ legacy suites: all PASS with zero failed suite checks.
- C++ and Vitis csim non-DIV normalized architecture/event/cycle traces: 21/21 PASS.
- BOOM full-program architectural diff: 10/10 PASS.
- Partial-order regression: PASS with no exposed violation.
- Canonical csynth: 8/8 complete; `CORE_CYCLE` remains unpipelined.

No frozen expected trace, M2C evidence, or M3A evidence was overwritten.
