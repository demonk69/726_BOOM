# Gate 4.1 M3B Results

`M3B_INT_DIVIDER_INTEGRATION_VERIFIED=true`

`READY_FOR_M3C_RV64M_FINAL=true`

`GATE4_1_RV64M_VERIFIED=false`

## Required Answers

1. Divider state: one `DividerExecutionState` member in canonical `ExecuteState`, containing M3A arithmetic state, `MicroOp`, ROB index, pdst, allocation ID, and branch mask.
2. Reset: `RESET_CONTROL` and `RESET_EXECUTE` call `divider_reset` and invalidate the token/identity; runtime reset cancels busy and pending work.
3. INT request ready: DIV/REM is selectable only when `divider_request_ready`, no token is active, branch/reset/exception permits it, and a live busy ROB entry owns the nonzero allocation ID. IQ removal occurs only on accepted issue followed by `divider_accept`.
4. Divider busy concurrency: ordinary ALU, MUL, branch, MEM, and load response continue. Only another DIV/REM is blocked. Pending response temporarily reserves the next empty INT execute slot.
5. Response path: M3A response is copied into lane 1 `AluResult`, then captured only by existing `completion.int_execute`.
6. INT conflict arbitration: existing INT slot owner wins; Divider response holds stable; held Divider publishes into the next empty slot; W4 then uses existing oldest-ROB-age/source tie-break arbitration.
7. Completion sources: 3.
8. PRF write ports: 2.
9. Wakeup/bypass: 3/3.
10. ROB complete sources: 3.
11. Branch kill: matching younger branch mask cancels active or pending Divider state; correct/older resolution only clears its mask.
12. Exception kill: global precise-exception flush conservatively cancels Divider work; divide-by-zero is a result, not an exception.
13. Reset mid-divide: verified in software and generated RTL with no post-reset response.
14. Stale allocation: checked at request, every active execute cycle, and publication; ROB reuse has zero side effect.
15. Dropped: 0.
16. Duplicate response/writeback/ROB complete: 0/0/0.
17. Directed: 167/167 integration checks PASS; M3A remains 104/104 PASS.
18. Integration random: 128 seeds x 1024 cycles PASS; 2857 accepted Divider tokens and zero invariant failures.
19. Focused RTL: 26/26 PASS.
20. Full-core programs: native C++ 10/10, Vitis csim 10/10, generated `boom_core_top` RTL 10/10 PASS.
21. Gate 3.9 XSim: 49/49 PASS; five added Divider reset cases PASS.
22. Old regressions: M1/M2/M3A/W3/W4/reset/traces/full-program/partial-order all PASS.
23. Full-core resources: `boom_core_top` 123520 LUT, 26815 FF, 16 BRAM, 3 DSP; `synth_core_step_top` 116249 LUT, 26408 FF, 16 BRAM, 3 DSP.
24. DSP: 3 in both full-core tops; standalone Divider uses 0.
25. Estimated period: 6.341 ns for both full-core tops; execute is 6.411 ns; Divider is 5.734 ns.
26. Critical path: full-core estimate remains the M2C 6.341 ns product path; Divider acceptance is the standalone Divider's 5.73 ns state and does not become the full-core critical path.
27. `M3B_PPA_BLOCKER=false`.
28. Final M3B state: `M3B_INT_DIVIDER_INTEGRATION_VERIFIED=true`.
29. `READY_FOR_M3C_RV64M_FINAL=true`.
30. `M009=PARTIALLY_VERIFIED`.
31. `M014=VERIFIED`.
32. `READY_FOR_OFFICIAL_GATE_3=false`.

## Guardrails

`CORE_CYCLE` is not pipelined. No DATAFLOW, false-dependence, complete-array-partition, or M3C PPA directive was introduced. `src/boom_all.cpp` was pre-existing dirty state and remained excluded and untouched by M3B. No frontend, full LSU, cache, MMU, or FPU work was started.
