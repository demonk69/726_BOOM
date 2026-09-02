# Gate 5.4 P1 Regression Preservation

| Check | Result | Evidence |
|---|---:|---|
| merged translation unit compile | PASS | `regression/w3/logs/merged_compile.log` and P1 build object |
| Gate 5.3 focused relevant preservation | PASS | standalone packet/native tests plus unchanged protected hashes |
| W3 native software | 400/400 PASS | fresh P1-isolated W3 logs; exact suite accounting rerun |
| RV64M native | directed/random and 15/15 PASS | `regression/m3c/logs/` |
| synth_frontend_top | PASS | 7824 LUT, 4430 FF, 6.071 ns, PipelineType=no |

The legacy W3 launcher's optional Vitis csim trace phase was attempted but its pre-Gate-5.3 Tcl source list omits `fetch_buffer.cpp` and `fetch_packet.cpp`, so it stopped at link. That optional phase is not a P1 requirement and is not used as evidence. The requested W3 400/400 native suites ran before that phase and were independently recounted. No product source was changed to repair the historical launcher.

Hashes of `frontend.cpp`, `fetch_packet.cpp`, `fetch_buffer.cpp`, `decode.cpp`, `branch.cpp`, `rob.cpp`, and `execute.cpp` exactly match baseline. The pre-existing dirty `src/boom_all.cpp` hash also matches and the file remains excluded.
