# PF1 Exception Contract

- Take point: only a valid, non-busy ROB head with `exception=true`.
- Backpressure: take is atomic with exception commit-trace capacity; a full
   commit stream leaves the owner and architectural state unchanged.
- Event: exactly one `CommitEntry` with the fault PC/instruction/cause and one
  same-cycle `ExceptionCommitEvent` containing PC, cause, tval, and target.
- CSR update: `mepc=fault_pc`, `mcause=fault_cause`, and `mtval` is the fault PC
  for instruction address faults or the illegal instruction bits for cause 2.
- Privilege: `MIE` moves to `MPIE`, `MIE` clears, `MPP` receives prior privilege,
  and execution enters M mode.
- Redirect: target is aligned `BOOM_TRAP_VECTOR` (`0x10100`); exception redirect
  outranks a same-cycle younger branch redirect and loses to runtime reset.
- Terminal outputs: a recoverable exception never asserts `io_trap`; the
  existing ECALL host convention remains terminal and unchanged.
