# PF6 Critical Path Reproducibility

- Fresh `boom_core_top` estimated period: 6.341 ns.
- PF5 accepted estimate: 6.341 ns; delta: 0.000 ns.
- `CORE_CYCLE` pipeline field: `no`.
- The full-core resource/timing result is byte-independent fresh synthesis output.
- Limiting function/state: `execute_module`, State 11 (`SV=10`), 6.34 ns.
- Startpoint: integer PRF bank 1 read in `include/boom_state.hpp:387`.
- Endpoint: multiplier result `ret`, called by `src/execute.cpp:152`
  (`src/boom_core_merged.cpp:3194`).
- Operation chain reported by Vitis HLS: PRF load (1.24 ns), mux before the
  `rs2` phi (0.574 ns), `rs2` phi (0 ns), multiply (4.53 ns).
- The chain is execute operand-to-multiply datapath, not Commit training to
  predictor forwarding to Frontend.

Fresh raw provenance:

- `.autopilot/db/execute_module.verbose.sched.rpt:1598-1603` contains the
  state delay and complete source-mapped operation chain.
- `.autopilot/db/execute_module.verbose.bind.rpt:691-713` independently maps
  State 11, the PRF loads, `rs2`, and operand selection operations.
- Both files are under the fresh PF6 canonical `boom_core_top` solution in
  `/tmp/boom_hls/pf6/canonical`; `/tmp` remains ephemeral build provenance.
