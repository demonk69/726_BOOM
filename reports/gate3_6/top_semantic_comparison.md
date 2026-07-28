# Gate 3.6 Top Semantic Comparison

## Conclusion

`synth_core_step_top` and `boom_core_top` both execute `boom_core_step`, but they are not interchangeable tops. The direct step top is a diagnostic `ap_ctrl_hs`/FIFO design with `seed_pc` and `observable`; the product top is an AXIS, `ap_ctrl_none`, free-running design with scalar status outputs and a persistent internal stream bundle.

`boom_core_step_top` is the valid one-call control experiment for the product interface. Its 83353 LUT result is only 67 LUT above `boom_core_top`, which directly rules out the infinite `while (true)` loop as the source of the 37936-LUT difference between `synth_core_step_top` and `boom_core_top`.

## Call Structure

| Function | Calls | State | Loop |
|---|---|---|---|
| `boom_core_top` | `boom_core_cycle_io` | function-local static `BoomCoreState` | infinite `CORE_CYCLE`, not pipelined |
| `boom_core_step_top` | `boom_core_cycle_io` once | independent function-local static `BoomCoreState` | none |
| `synth_core_step_top` | `boom_core_step` directly | independent function-local static `BoomCoreState` | none |
| `boom_core_cycle_io` | stream ferry, `boom_core_step`, scalar output sampling | referenced caller state and local stream bundle | none |
| `boom_core_step` | CSR, branch, LSU, frontend, decode, rename, ROB allocate, issue, execute, commit | referenced state, updated in place | none |

## Semantic Differences

| Item | Product tops | Direct diagnostic step top | Impact |
|---|---|---|---|
| Interface protocol | AXIS streams, `ap_none` scalars, `ap_ctrl_none` | FIFO streams, `ap_vld` observable, `ap_ctrl_hs` | Not interface-equivalent |
| Status outputs | success, halted, trap, cycle-valid, cycle, instret | cycle only through `observable` | Diagnostic output is narrower |
| PC seed | none | odd `seed_pc` may overwrite PC | Diagnostic-only behavior |
| Input stream ferry | checks external nonempty and internal nonfull | checks external nonempty only | Backpressure structure differs |
| Persistent stream bundle | one local `PipeSignals` object survives free-running loop iterations | recreated per invocation in C++ semantics | Different wrapper lifetime |
| State ownership | one static `BoomCoreState` per top | one independent static `BoomCoreState` | No shared or duplicate state inside one selected top |
| Reset pragma | static state explicitly reset in product tops | no reset pragma in diagnostic top | T4 directly isolates 37684 LUT of reset-triggered elaboration |

## Direct Evidence

- Product wrapper and interface: `src/boom_core_top.cpp:8-81`.
- One-call product wrapper: `src/boom_core_top.cpp:84-116`.
- Direct diagnostic wrapper: `src/synth_module_tops.cpp:285-302`.
- In-place cycle order: `src/boom_core_step.cpp:20-35`.
- No whole-state copy exists in the active cycle path; all modules receive `BoomCoreState&`.

Strict BOOM cycle equivalence remains `INSUFFICIENT_EVIDENCE`. Gate 3.6 compares HLS tops and the accepted HLS trace baseline; it does not upgrade official BOOM cycle equivalence.

The T4 no-reset product diagnostic retains the product interface, free-running loop, persistent state, and cycle wrapper while removing only the state reset pragma. It synthesizes at 45602 LUT with 342 automatic partitions. This closes the semantic audit: reset behavior, rather than loop count or duplicate state, is the dominant synthesis distinction.
