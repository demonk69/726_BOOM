# M3A Source Scope

## In Scope

- `include/divider.hpp`
- `src/divider.cpp`
- `tb/differential/divider_unit_tests.cpp`
- `tb/differential/divider_random_tests.cpp`
- `tb/differential/divider_protocol_random_tests.cpp`
- Independent `synth_divider_top` in `src/synth_module_tops.cpp`
- Canonical insertion of `src/divider.cpp` by `scripts/generate_merged.sh`
- Regenerated `src/boom_core_merged.cpp`
- Divider documentation and `reports/gate4_1/m3/m3a/`

## Frozen And Out Of Scope

- No execute integration; `src/execute.cpp` must remain hash-identical.
- No completion integration; `src/completion.cpp` must remain hash-identical.
- No ROB, LSU, issue, or frontend changes.
- No INT-lane, W4 completion, PRF write, wakeup, or bypass changes.
- No full-core RTL or full-core csynth runs.
- No `CORE_CYCLE` pipeline, `DATAFLOW`, or false-dependence directives.
- `src/boom_all.cpp` remains excluded, dirty before M3A, and untouched.

The canonical merged order is frontend, decode, rename, completion, ROB, issue, multiply, divider, execute, branch, LSU, commit, CSR, core step, reset, core top, and synthesis wrappers.
