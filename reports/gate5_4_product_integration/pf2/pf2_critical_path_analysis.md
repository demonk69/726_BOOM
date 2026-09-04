# PF2 Critical Path Analysis

Full-core estimated period remains 6.341 ns, unchanged from PF1. The predictor does not form a combinational `predecode -> BIM -> mask -> Fetch Buffer` path: conditional request and response are separated by the P2 held-response state. `CORE_CYCLE` is unpipelined.

The standalone canonical `synth_frontend_top` reports 7.621 ns and the focused `synth_pf2_predictor_frontend_top` reports 6.375 ns, so Frontend timing remains a review item, but the canonical product `boom_core_top` remains within the 6.5 ns hard gate. The full-core estimate remains the existing execute-side 6.341 ns path; Predictor did not become the full-core reported period.
