# Predictor Cycle Contract

The logical interface is a blocking step protocol:

- If idle and not reset, `req_ready=true`; a valid request is accepted on logical call N.
- The response is visible on logical call N+1.
- While `resp_valid=true`, the response is stable until `resp_ready` is sampled.
- A pending response blocks new requests, including on its consume call. There is no same-call turnover.
- Therefore N accepted requests require exactly N accept calls and N response calls in sequence; the first response is N+1 relative to its acceptance.
- Reset suppresses ready/valid and clears a pending response.

This is the P0 fixed-one-cycle logical model. It is not a claim that generated RTL has one physical clock of top-level transaction latency. Vitis reports best-case latency 3, minimum II 4, and `PipelineType=no` for the generated wrapper; XSim verifies its call-to-call protocol.
