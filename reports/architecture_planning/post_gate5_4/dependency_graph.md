# Full LSU Dependency Graph

`Gate5.4 release -> T0 ownership/timing review -> L1 canonical parameterized queues -> L2 forwarding -> L3 multi-outstanding loads -> L4 speculation/violation/replay -> L5 product closure`

Timing repair T0R may proceed after T0 and in parallel with L1, but must close before L2. DCache and MMU are independent future architecture gates; neither is a dependency to implement inside the LSU.
