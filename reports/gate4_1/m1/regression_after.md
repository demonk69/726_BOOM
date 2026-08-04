# Gate 4.1 M1 Regression After

Result: **PASS** for M1 decode and all required preservation gates.

- Independent M decode/classification: 42/42 vectors PASS.
- Valid RV64M decode: 13/13 PASS.
- `rd=x0` M decode: 4/4 PASS.
- Illegal near-miss decode: 10/10 PASS.
- RV64I OP/OP-32 collision preservation: 15/15 PASS.
- W3 canonical software: 15/15 suites, 400/400 checks PASS.
- W4 cumulative directed: 95/95 PASS.
- Reset architecture: 14/14 PASS.
- W4 persistent random: 128/128 seeds, 16,384/16,384 cycles PASS.
- C++/Vitis csim normalized trace pairs: 7/7 PASS; architecture/event/cycle checks 21/21 PASS.
- Frozen W4 trace byte identity: 14/14 files PASS.
- Full-program architectural diff: 10/10 PASS.
- Partial order: 7/7 PASS.
- Merged generation and compile checks: 4/4 PASS.
- Conservative Vitis HLS 2021.2 csynth: 3/3 targets PASS.

Detailed preservation output is under `w4_preservation/`. No existing trace expectation was changed.
