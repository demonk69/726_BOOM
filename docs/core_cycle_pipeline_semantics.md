# CORE_CYCLE Pipeline Semantics

## Scope

Gate 3.7 evaluates an HLS loop-pipeline directive on `boom_core_top/CORE_CYCLE`. It does not enable the directive in the accepted configuration and does not claim that an HLS iteration pipeline is equivalent to the processor's internal pipeline.

The accepted top retains:

- one static `BoomCoreState`
- one persistent `PipeSignals` bundle
- whole-state HLS reset directive
- AXIS request, response, and commit streams
- `ap_ctrl_none` free-running control
- disabled `CORE_CYCLE` pipeline

## Two Different Pipelines

### HLS Loop Pipeline

Applying `PIPELINE` to `CORE_CYCLE` permits Vitis HLS to overlap operations from C++ loop iterations `k`, `k+1`, and later iterations. Each C++ iteration represents one logical processor cycle, so overlap is correct only when all state, control, memory-port, FIFO, and output-backpressure dependencies preserve the original transition order.

### Processor Internal Pipeline

The modeled processor pipeline is already represented by persistent state. For example, execute writes `state.execute.alu_results` in iteration `k`; branch and LSU consume that value in iteration `k+1`. HLS iteration overlap does not add another architectural stage and cannot be assumed safe merely because BOOM is a pipelined processor.

## Logical Cycle Order

`boom_core_cycle_io` performs:

1. Import available IMEM and DMEM responses into persistent internal streams.
2. Call `boom_core_step` once.
3. Export at most one IMEM request, one DMEM request, and one commit trace.
4. Publish success, trap, cycle, and instret scalar values.

`boom_core_step` mutates one state object in this fixed order:

1. Clear one-cycle control pulses.
2. CSR cycle update.
3. Branch resolution and mispredict recovery.
4. LSU response/queue processing.
5. Frontend.
6. Decode.
7. Rename.
8. ROB allocation.
9. Issue.
10. Execute.
11. ROB completion and commit.

There is no current/next state double buffer. A later iteration may not read or overwrite a field until all earlier readers and writers required by this order have completed.

## Dominant Recurrences

| Recurrence | Required Order | Semantic Risk |
|---|---|---|
| execute result | `execute(k)` -> `branch/LSU(k+1)` -> `execute(k+1)` overwrite | redirect, memory operation, or flush can be lost |
| frontend PC/request | request/response state(k) -> frontend(k+1) | duplicate fetch IDs, stale response acceptance, wrong redirect |
| rename state | map/free/busy/snapshot(k) -> rename/recovery(k+1) | wrong physical mapping or leaked/reused register |
| ROB | allocate/complete/commit(k) -> scan/allocate/commit(k+1) | corrupt circular queue or retire wrong entry |
| issue queue | kill/wakeup/select/compact(k) -> queue(k+1) | duplicate, drop, or issue killed uop |
| LSU | request transaction(k) -> matching response(k+L) | duplicate transaction ID or wrong load completion |
| branch state | tag/snapshot/allocation list(k) -> release/recovery(k+N) | restore wrong snapshot or leak tag |
| CSR counters | cycle/instret(k) -> increment/output(k+1) | cycle and retirement count mismatch |
| internal streams | FIFO write/read/full(k) -> operation(k+1) | request reorder, loss, or deadlock |
| external AXIS backpressure | state transition(k) -> blocking output acceptance -> iteration(k+1) | restart/repeat state-changing work while output is stalled |

## Backpressure Semantics

The output writes in `boom_core_cycle_io` are blocking AXIS writes. In the conservative sequential loop, a blocked `imem_req_out`, `dmem_req_out`, or `commit_trace_out` prevents the next logical cycle from starting. A pipelined schedule must preserve this boundary; it may not start another state-changing transition while an earlier iteration is blocked in a way that changes observable request, commit, or counter ordering.

The internal commit stream is not architectural backpressure: `rob_commit_module` can retire and increment instret when the internal trace FIFO is full, omitting the trace. This existing behavior makes RTL-level backpressure validation mandatory for any pipeline candidate.

## Reset Semantics

The source reset directive remains mandatory in every experiment. Native C++ and Vitis csim do not test generated `ap_rst_n` behavior. Gate 3.7 therefore does not accept a pipelined result without a custom RTL test that asserts reset during execution and verifies persistent state, FIFOs, PC, queue pointers, cycle, and instret.

Existing generated RTL requires a separate reset audit: several state memories use power-on initialization and do not visibly consume their reset port. Until the RTL test closes this, mid-run whole-state reset is `NOT_VERIFIED` for both conservative and experimental RTL.

## Directive Policy

Gate 3.7 permits only an explicit loop-pipeline directive on an isolated synthesis solution. It does not use:

- `DEPENDENCE false` on `BoomCoreState`
- false inter-iteration dependencies on ROB, IQ, LSU, PRF, maps, free lists, or branch state
- whole-state replication or double buffering
- reset removal
- stream full/empty removal
- queue-depth or field-width reduction
- architecture-width changes

An achieved II is a scheduler result, not functional proof. A candidate remains synthesis-only until C/RTL cosim, runtime reset, stream backpressure, trace comparison, BOOM architectural diff, and partial-order checks pass.

## Acceptance Consequence

`PIPELINE II=1` is not assumed semantically valid. It is feasible only if Vitis reports a legal schedule without suppressing true dependencies and RTL evidence demonstrates one architectural transition per accepted logical cycle with identical state, stream, reset, and trace behavior.

See `reports/gate3_7/loop_carried_dependency_inventory.csv` and `reports/gate3_7/pipeline_feasibility_analysis.md`.
