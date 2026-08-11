# Frontend Verification Plan

F0 was design-only. Gate 5.1R native and UBSan tests pass, the verification-capable generated RTL passes all 33 mandatory checks, and all nine canonical csynth targets pass. The wrapper calls canonical Frontend/Decode logic and is not a product synthesis entry.

Gate 5.1 emphasizes protocol adversarial timing: arbitrary request backpressure and response delay; wrong ID/epoch/address; redirect and reset with requests outstanding; redirect concurrent with response; repeated redirects; fault propagation. Assertions cover at most one pending request, stable request payload under backpressure, no stale instruction reaching decode, and redirect priority.

Gate 5.2 exhaustively tests legal/illegal RVC expansion, 2-byte alignment, cross-word assembly, mixed streams, and RVC branch/JALR PCs. Gate 5.3 tests packet masks, partial packets, buffer full/empty/wrap/flush and one-wide drain. Gate 5.4 tests FTQ allocation/read/update/wrap and redirect pruning. Gates 5.5-5.6 test predictor hit/miss/update/history/RAS repair. Gate 5.7 tests ICache hit/miss/refill/eviction/fault/FENCE.I.

Protection checks compare canonical source hashes and assert unchanged decode/dispatch/commit widths, ROB/IQ/PRF capacities, trace format, expected files, and accepted directive configuration.

Gate 5.1R separates architectural-call throughput from the `ap_ctrl_hs` diagnostic wrapper. The canonical Frontend now sustains one request/response/dispatch per native call; the wrapper remains interval 3 and does not imply one physical-clock full-core throughput. W3 is 400/400, focused RTL is 33/33, and full-core RTL is 49/49. Therefore `GATE5_1_THROUGHPUT_BLOCKER=false`, `GATE5_1_FOCUSED_RTL_VERIFIED=true`, and `READY_FOR_GATE5_2_RVC=true`.
