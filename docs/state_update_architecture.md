# State Update Architecture

Gate 3.2 removes the synthesis-hostile whole-state update pattern:

```cpp
BoomCoreState next_state = state;
...
state = next_state;
```

The baseline now keeps one persistent `BoomCoreState` and updates it in the existing serialized cycle order. This preserves all frozen architectural traces while avoiding per-cycle copies of ROB, IQ, PRF, LDQ/STQ, and branch snapshot storage.

## Cycle Priority

The current cycle order is intentionally unchanged for trace preservation:

| Priority | Function |
|---:|---|
| 1 | clear per-cycle `global_flush`, `io_success`, `io_trap`, and `brupdate` flags |
| 2 | `csr_module` updates cycle count |
| 3 | `branch_module` observes previous execute results and forms redirect events |
| 4 | `lsu_module` consumes memory responses and issues conservative memory requests |
| 5 | `frontend_module` handles flush/redirect and imem response/request state |
| 6 | `decode_module` decodes the visible fetch packet |
| 7 | `rename_module` maps sources/destinations and allocates physical registers |
| 8 | `rob_allocate` allocates renamed uops |
| 9 | `issue_module` dispatches/selects ready uops |
| 10 | `execute_module` produces ALU/branch/memory address results |
| 11 | `rob_commit_module` completes/commits in order and emits store/commit traces |

## Ownership Split

| State Area | Writer |
|---|---|
| Frontend state | `frontend_module` |
| Decode output | `decode_module` |
| Rename map/free-list/output | `rename_module`, committed-map updates in `rob_commit_module` |
| ROB allocation/completion/flush | `rob.cpp` |
| Commit, stale-pdst recycle, store commit, commit trace | `commit.cpp` |
| Issue queue and issue outputs | `issue_module` |
| Execute results and integer writeback | `execute_module` |
| LSU queues, load completion, memory requests | `lsu_module`, store commit request in `rob_commit_module` |
| CSR cycle/instret | `csr_module`, `rob_commit_module` |

This is still a conservative serialized HLS model, not a complete BOOM microarchitectural timing model. It establishes a stable, synthesizable baseline before branch snapshot implementation and before any PPA directive work.

## Gate 3.2 Evidence

- Native regressions pass: directed `25/25`, Gate 1 `13/13`, LSU `14/14`.
- HLS C++ and Vitis csim complete traces are byte-identical to the frozen baseline.
- Full loaded-program BOOM-vs-HLS architectural diff remains `10/10 PASS`.
- Baseline `boom_core_top` csynth completes without `CORE_CYCLE` pipeline.
- `BoomCoreState next_state = state` no longer appears in `src/`.
