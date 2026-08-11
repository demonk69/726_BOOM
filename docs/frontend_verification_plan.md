# Frontend Verification Plan

Tests are planned, not executed in F0. Every gate uses directed C++ model tests, focused HLS csim, focused generated RTL simulation, and trace comparison where architectural behavior changes. Full-core synthesis is excluded from F0.

Gate 5.1 emphasizes protocol adversarial timing: arbitrary request backpressure and response delay; wrong ID/epoch/address; redirect and reset with requests outstanding; redirect concurrent with response; repeated redirects; fault propagation. Assertions cover at most one pending request, stable request payload under backpressure, no stale instruction reaching decode, and redirect priority.

Gate 5.2 exhaustively tests legal/illegal RVC expansion, 2-byte alignment, cross-word assembly, mixed streams, and RVC branch/JALR PCs. Gate 5.3 tests packet masks, partial packets, buffer full/empty/wrap/flush and one-wide drain. Gate 5.4 tests FTQ allocation/read/update/wrap and redirect pruning. Gates 5.5-5.6 test predictor hit/miss/update/history/RAS repair. Gate 5.7 tests ICache hit/miss/refill/eviction/fault/FENCE.I.

Protection checks compare canonical source hashes and assert unchanged decode/dispatch/commit widths, ROB/IQ/PRF capacities, trace format, expected files, and accepted directive configuration.
