# Frontend Parameterization Policy

Architectural parameters define observable capacity or behavior: fetch width/bytes, Fetch Buffer and FTQ depth, IMEM outstanding count, epoch width, RAS/predictor geometry, RVC enable, and ICache geometry. They require functional tests and configuration evidence.

HLS parameters define implementation only: storage binding, limited partition factor, local pipelining, inline boundaries, resource sharing, and reset implementation. They must not change architectural traces. No directives are added in F0. `DATAFLOW`, false dependence, complete array partition, raw `boom_core_step` synthesis, and full-core synthesis remain prohibited.

Gate 5.1 freezes only single outstanding, epoch width, reset PC, redirect ordering, and single-instruction ready/valid behavior. Predictor and ICache parameters may remain unknown because they are not dependencies. Every later parameter change requires focused csim/RTL equivalence and PPA comparison against the accepted Gate 4.1 configuration.
