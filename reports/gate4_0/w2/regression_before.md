# Gate 4.0 W2 Regression Baseline

Immutable W1 commit: `fa3dbcc8d9559d0da5a0aa8d401816404124fb99`.

No tests were rerun for this freeze; outcomes below are parsed from committed W1 evidence.

| Suite | Outcome | Passed | Failed | Runs | Evidence SHA-256 |
|---|---|---:|---:|---:|---|
| Directed | PASS | 25 | 0 | 1 | `b2ac6f5e20c5e43b073ed88aeedf8967e5679dd4a8f7b8e083b8790db2931ab3` |
| Gate 1 regression | PASS | 13 | 0 | 1 | `d02ec2385a6480b1ddab2a86d20047490e56fc30895e7bcc78df6ef0326fd061` |
| IQ compaction | PASS | 10 | 0 | 1 | `e4ee91f1845a638ac64d9058c794f5888dabf2abaea87af05949432b7c260bda` |
| Branch snapshot | PASS | 30 | 0 | 1 | `412affea88bd48d63154c7f06ecc0f2cd6bd1ca16df3f49c787771dcfed1a6dd` |
| Branch randomized | PASS | 42 | 0 | 21 | `e090f56d75582bf4c4e80424f5f7261c9d363d36c1dfd2ac413823bdbe9248c7` |
| Minimal LSU | PASS | 14 | 0 | 1 | `74df03b790dbf3842ce18c4087b96631ee38b42f49881781d8320784164f90c9` |
| Reset architecture | PASS | 14 | 0 | 1 | `7b0ae4133dd0d4d0b9846969e267ccb39d8f10ad780816a159f5f01bac4e94e8` |
| W1 lane interface | PASS | 1 | 0 | 1 | `d43bd0e56fd747f34a18a95dff1f25cc218441b617b7f1f4289f2ca037f0d1b3` |

## Trace And Synthesis Outcomes

- W1 frozen C++/csim trace comparison: **PASS**, 10/10 byte-identical; evidence `reports/gate4_0/w1/regression/trace_diff.md`, SHA-256 `c48cf4503e9aa8e18823b284c78e336b1777adc01c0359b4a3912d7950919a3a`.
- W1 full-program architectural diff: **PASS**, 10/10; evidence `reports/gate4_0/w1/regression/full_program_architectural_diff.csv`, SHA-256 `d25515447134e17342ef673ef61be38a00bb3c6a78111417cf77edfbbad4b3f4`.
- W1 conservative csynth: **PASS**; resources and report identities are frozen in `w1_resource_baseline.csv`.

## Summary

Recorded unit/regression assertions: **149 passed, 0 failed**. Randomized branch coverage comprises 21 runs and 42 passing assertions.
