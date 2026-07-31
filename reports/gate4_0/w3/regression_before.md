# Gate 4.0 W3 Regression Baseline

Immutable W2 commit: `210ad1900457b073806a54617d313a2c61a14e21`.

No tests were rerun. Every outcome below was parsed from a committed W2 blob.

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
| W2 dual-grant directed | PASS | 28 | 0 | 1 | `7cfcc059e732ef14014a254f331c3d040954bc8e0dc3e1b8e11672c5a18f1ef4` |

## Campaign And Integration Evidence

- Recorded source assertions: **177 passed, 0 failed**; W2 directed selection is **28/28 PASS**.
- W2 random differential: **64 seeds**, **2048 cycles**, **63 dual grants**, **382 accepted**, **424 retained**, **0 dropped**; evidence SHA-256 `b90fa70e4529a2fd29bd98a78a66606f5561b0899577c4fe2374102663a5ba32`.
- Frozen HLS C++/csim trace comparison: **10/10 byte-identical**; evidence SHA-256 `04a54b9f18f14bb512bf323198b9a92b1f83d1f1a4b49ed2ab3f5b3c38972106`.
- Full-program architectural comparison: **10/10 PASS**; evidence SHA-256 `d25515447134e17342ef673ef61be38a00bb3c6a78111417cf77edfbbad4b3f4`.
- Dedicated synthesized issue-selection RTL: **5/5 PASS**; evidence SHA-256 `905c5572a32ec1dd05fe84d2bf75f6adb392e463df2cb50281543324f86e1667`.
- Full generated-core XSim matrix: **49/49 PASS**; evidence SHA-256 `d783a104405b15f6607d762d3d21a9fa1163f07017e2deef10dfcfc90001569a`.
- W2 synthesis resources are frozen in `w2_resource_baseline.csv`.

## Qualification

The official Chipyard/FESVR/DRAMSim path remained unavailable. This freezes verification of the supported HLS subset and generated RTL, not full BOOM equivalence.
