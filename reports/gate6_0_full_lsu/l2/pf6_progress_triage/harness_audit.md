# PF6 Harness Audit

- Waveform dumping: disabled; no `log_all_signals`, recursive `log_wave`, or
  explicit WDB population is present.
- Logging: event/final-summary driven. No per-cycle `$display`, `$fwrite`, or
  full-state comparison is present.
- Clock: `forever #5 clk = ~clk`; observed simulation time and edges advance.
- Reset: active-low wiring is correct and `reset_completed` becomes one.
- Top control: `ap_ctrl_none`; no `ap_start` drive is required.
- IMEM/DMEM/trace ready-valid models are the same established models used by
  other full-core tests. The repaired smoke observed fetch, response, commits,
  and tohost termination.
- Termination remains `tohost_seen && tohost_commit_seen`, with the existing
  fault-specific completion condition and unchanged oracle checks.

The stale reference was `dut.ap_CS_fsm_state34`. In current PF6 RTL, state 34
starts `boom_core_reset_step`, so `state34 && reset_completed` can never serve
as a normal cycle observer. Current state 35 is the subcall wait state and is
the exact control-flow equivalent of state 34 in the previously passing RTL.

All remaining PF6 hierarchy references compiled and elaborated successfully,
including the current cycle-IO instance `fu_7823` and commit instance
`fu_3682`.

`PF6_HIERARCHY_REFERENCES_VALID=true`

`PF6_CLOCK_PROGRESS=true`

`PF6_RESET_RELEASED=true`

`PF6_TOP_CONTROL_STARTED=true`

`PF6_TERMINATION_CONDITION_VALID=true`
