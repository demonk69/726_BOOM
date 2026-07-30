# R1 RTL Comparison

- XSim matrix: 49/49 PASS.
- Completed reset releases: 60; interrupted releases: 6; failures: 0.
- First post-reset fetch PC: `0x10040`.
- Baseline reset-to-first-fetch: 688 cycles.
- R1 reset-to-first-fetch: 886 cycles, delta +198 (worse).
- Normal architectural commit/tohost values remain correct, but full external event order is only 6/7 identical because `load_store` moves a load commit relative to an IMEM request.
- Normal event cycles relative to first fetch: 0/7 exact.
- Absolute event cycles: 0/7 exact.

Decision: `REJECTED_RESET_LATENCY_AND_NORMAL_CYCLE_CHANGE`.

The top-level `io_instret` port is generated as an input by the existing reference signature, so it cannot be independently observed at the RTL pin. Internal `state_csr_instret` advances through the same commit operation represented by the compared commit trace; XSim commit counts and reset tests remain consistent. This interface limitation prevents a stronger independent pin-level `io_instret` claim.
