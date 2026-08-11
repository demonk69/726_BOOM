# Gate 5.1R R3 Throughput Analysis

R3 removes the Frontend architectural bubble with a next-state transition only. A matching response can be consumed, converted into the held instruction, and followed by the next request in the same `frontend_module` call when Decode is ready. The 128-call native run changes from one request/response/dispatch every two calls to one per call. The corrected ROB-full fixture restores W3 to 400/400.

The accepted synthesis configuration is S0: no Frontend or top-level pipeline directive. The focused `synth_frontend_top` is an `ap_ctrl_hs` diagnostic transaction wrapper, not the free-running product. Its csynth interval remains 3 and its measured request interval is 3 clocks with zero-latency responses or 6 clocks with ordinary delayed responses. Those clocks must not be reported as canonical architectural cycles. Conversely, the native one-call interval must not be reported as a one-`ap_clk` product claim.

S1 is rejected: Vitis reports a carried `state_frontend_pc` dependence and reaches top Final II=2, not the requested II=1. S3 is also rejected: inlining obtains a paper Final II=1 but overlaps persistent state transactions and regresses actual RTL scheduling. Neither candidate may be accepted or used for final RTL.

R3 therefore closes the avoidable Frontend state-machine bubble while preserving the conservative product schedule. `GATE5_1_THROUGHPUT_BLOCKER=false` means no known avoidable Frontend architectural-call bubble remains; it does not claim a pipelined `CORE_CYCLE` or one instruction per physical `ap_clk`.

Detailed values are in `throughput_layers.csv` and `scheduling_experiments.csv`.
