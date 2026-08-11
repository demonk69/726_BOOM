# Gate 5.1R R4 Critical Path Analysis

The canonical `boom_core_top` estimate is 6.341 ns. Its longest child schedule is `execute_module`, also 6.341 ns, with variable 6-40 cycle latency and no pipeline. The standalone `synth_execute_top` estimate is 6.411 ns. The dominant cone remains Execute state and operand/result selection around the replicated integer PRF and RV64M execution; Gate 5.1 Frontend matching does not become the product critical path.

`frontend_module` is a one-state, unpipelined schedule estimated at 5.993 ns. Its longest state includes response FIFO access, fetch ID/epoch/address comparisons, redirect selection, held-uop update, and request generation. It is 0.348 ns below the product estimate.

These values are Vitis HLS 2021.2 scheduling estimates, not post-route STA. The 6.341 ns product estimate satisfies the 6.5 ns Gate 5.1 limit. `CORE_CYCLE` is explicitly `Pipelined=no`; no DATAFLOW, false-dependence, or explicit complete-partition directive is accepted.
