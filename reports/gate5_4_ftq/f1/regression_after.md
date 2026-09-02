# F1 Regression Preservation

| Check | Result |
|---|---|
| merged translation unit compile | PASS with existing warning exemptions |
| P1 directed/RVC exhaustive | 994 checks and 65,536 parcels, zero errors |
| P1 packet/random | 256x4096 packets and 1,000,000 words, zero errors |
| P2 directed | 5008/5008 PASS |
| P2 predecode composition | 10620/10620 PASS |
| W3 fresh native accounting | 400/400 PASS |
| RV64M native | 15/15 plus directed/random PASS |
| `synth_frontend_top` | PASS, 7824 LUT, 4430 FF, 0 BRAM/DSP, 6.071 ns |
| `synth_predictor_foundation_top` | PASS, canonical LUTRAM: 684 LUT, 465 FF, 0 BRAM/DSP, 2.989 ns |
| protected source hashes | identical |

The old W3 optional csim Tcl still omits Fetch Buffer/Packet sources and stops
after native acceptance, as already documented by P2. It is not part of the
required fresh W3 400/400 acceptance.
