# L1R2A Product Boundary Map

## Frozen Candidate

- Modular source/header aggregate: `b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618`.
- `src/boom_core_merged.cpp`: `76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c`.
- `src/boom_all.cpp`: `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- L1R2A changed only test wrappers, RTL testbench, runner, Tcl, and reports. Product sources are inputs, not edits.

## Boundaries

| Semantic boundary | Canonical implementation | Required state/identity | Focused path |
|---|---|---|---|
| LQ allocation/free | `enqueue_load`, `lsu_accept_completion`, `reclaim_ldq`, `lsu_finish_load_response` in `src/lsu.cpp:123-173,195-229` | slot, generation, ROB index, allocation ID, transaction ID, pending bit | PATH_A stateless sequence |
| SQ allocation/free | `enqueue_store`, `lsu_accept_completion`, `lsu_reclaim_store` in `src/lsu.cpp:82-139,176-185` | slot, generation, ROB index, allocation ID | PATH_A stateless sequence |
| SQ owner validation | `lsu_store_owner_matches` in `src/lsu.cpp:187-193` | exact owner tuple | PATH_A stateless sequence |
| Response accept/drop | `completion_from_load_response` in `src/completion.cpp:73-121`; `lsu_finish_load_response` in `src/lsu.cpp:153-173` | pending tuple, live ROB owner, live LQ owner, response-pending flag | PATH_A stateless sequence |
| Older-store blocking | `older_store_in_rob` and its issue gate in `src/lsu.cpp:13-24,44-79` | ROB head/order, valid store, target load index | PATH_A exact internal translation-unit boundary; composite `lsu_module` rejected by watchdog |
| Global flush cleanup | `clear_lsu_queues`, `lsu_module` in `src/lsu.cpp:26-42,232-244` | `global_flush`, LQ/SQ counts and pending tuple | PATH_A when flush is fixed; PATH_B for issue/commit composition |
| Branch-mask clear | `clear_resolved_masks_in_state`, `release_resolved_branch` in `src/branch.cpp:69-95,289-292` | resolved tag bit across ROB/LQ/SQ and pipeline holders | PATH_A branch image |
| Mispredict squash | `kill_lsu_state`, `kill_rob_younger_than`, `recover_mispredict`, `branch_complete_event` in `src/branch.cpp:157-204,258-310` | branch tag/mask, queue owner, pending tuple, ROB age | PATH_A branch image |
| Exception flush | exception recovery sets `global_flush` in `src/commit.cpp:24-83`; LSU consumes it in `src/lsu.cpp:232-236` | oldest exception, committed rename state, both queues and pending response | PATH_B canonical full-core sequence |
| Committed-store lifecycle | request/reclaim in `src/commit.cpp:223-255`; `lsu_reclaim_store` in `src/lsu.cpp:176-185` | committed ROB head, SQ exact owner, dmem request readiness | PATH_B canonical full-core sequence |
| Independent reset cleanup | `RESET_LSU` in `src/reset.cpp:244-270` | independent `lq_index` and `sq_index`, queue counts, pending tuple | PATH_B current-source asymmetric full-core RTL |

## Architecture Rules

- Every PATH_A image has automatic local state and one fixed operation sequence per invocation.
- No PATH_A image contains static mutable `BoomCoreState`, opcode interpreter, dynamic command array, or cross-call state.
- Synthesized wrappers expose raw function results and post-state. Expected values and PASS/FAIL decisions exist only in `rtl_tb/gate6_0/g6_l1r_stateless_pilot_tb.sv`.
- `tb/differential/g6_l1r_stateless_older_store_top.cpp` textually includes canonical source files so the real internal `older_store_in_rob` predicate is called in its defining translation unit. It does not reproduce the predicate.
- PATH_B means direct reuse of current-source canonical full-core RTL evidence or a future canonical full-core scenario. It is required where a narrow wrapper retains the whole reset switch or where commit/issue composition exceeds the resource policy.

## Rejected Boundaries

| Attempt | Result | Decision |
|---|---|---|
| Local `BoomCoreState` plus `boom_core_reset_step` | Hard-stop at 8,821,180 KiB RSS after 135 s | Reset cases remain PATH_B; do not retry wrapper |
| Local store/load plus `lsu_module` | Hard-stop at 8,896,420 KiB workspace after 141 s | Direct issue-scan composition remains PATH_B; use exact blocking predicate for focused identity semantics |
