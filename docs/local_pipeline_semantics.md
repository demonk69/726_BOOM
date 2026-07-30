# Local Pipeline Semantics

Gate 3.10 distinguishes HLS scheduling cycles from processor-cycle semantics. A local pipeline is legal only when its latency is either contained entirely in reset initialization or proven not to move any normal external event.

## Classes

| Class | Meaning | Pipeline policy |
|---|---|---|
| `LOCAL_COMBINATIONAL` | Pure helper within one processor cycle | experiment only with exact normal event-cycle proof |
| `RESET_ONLY` | Reachable only before reset completion | allowed; record reset latency separately |
| `TRUE_STATE_RECURRENCE` | Loop reads and writes cross-cycle processor state | prohibited |
| `EXTERNAL_HANDSHAKE` | AXIS/stream readiness, occupancy, or transfer timing | prohibited |
| `SAME_CYCLE_RECOVERY` | Branch, exception, or reset recovery must be same-cycle | prohibited |
| `INSUFFICIENT_EVIDENCE` | Available reports do not prove legality | not run |

`CORE_CYCLE` is never pipelined. No false-dependence directive, full-state copy, complete-state double buffer, capacity reduction, or trace expectation change is permitted.

## Gate 3.9 Findings

The measured 5.90 ns LSU load-extraction cone is locally combinational but is not a loop pipeline target. The execute multiply, free-list scan, issue selection, ROB commit, branch recovery, and LSU request selection have real state, ordering, recovery, or handshake constraints. The only legal directive experiment is the reset-only ROB validity sweep in `R1_RESET_INIT_PIPELINE`.
