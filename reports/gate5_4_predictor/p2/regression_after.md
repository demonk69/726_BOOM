# Gate 5.4 P2 Regression Preservation

| Check | Result | Evidence |
|---|---:|---|
| P1 directed rerun | 994 checks, zero errors | P1 canonical test/log evidence |
| P1 RVC exhaustive rerun | 65,536, zero errors | P1 canonical test/log evidence |
| P1 packet rerun | 256 x 4,096, zero errors | P1 canonical test/log evidence |
| P1 random rerun | 1,000,000, zero errors | P1 canonical test/log evidence |
| merged translation unit compile | PASS | `regression/w3/logs/merged_compile.log` |
| W3 native fresh accounting | 400/400 PASS | `regression/w3/logs/` |
| M3C native | 15/15 plus directed/random PASS | `regression/m3c/logs/` |
| synth_frontend_top preservation | PASS | 7,824 LUT, 4,430 FF, 0 BRAM/DSP, 6.071 ns, `PipelineType=no` |
| protected product hashes | identical | `source_hashes_before.txt`, `source_hashes_after.txt` |

The historical W3 launcher's optional csim phase still fails because its old Tcl source list predates required Fetch Buffer/Fetch Packet sources. It is not a P2 requirement and is not acceptance evidence. The required native 400/400 accounting and merged compile pass independently. No product source was changed to repair that launcher.

`synth_frontend_top` is a preservation top, not an integrated predictor measurement. No full-core PPA delta is claimed.
