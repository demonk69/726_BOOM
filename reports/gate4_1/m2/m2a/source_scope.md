# Gate 4.1 M2A Source Scope

## Allowed

- `include/mul.hpp`
- `src/mul.cpp`
- `src/synth_module_tops.cpp`, only for stateless `synth_mul_top`
- `scripts/generate_merged.sh`, only to add `src/mul.cpp` once
- generated `src/boom_core_merged.cpp`
- M2A tests, scripts, reports, and multiply documentation

## Frozen

- `src/execute.cpp`
- `src/completion.cpp`
- `src/rob.cpp`
- `src/lsu.cpp`
- frontend, rename, issue, PRF, lane, completion, and `CORE_CYCLE` behavior
- existing trace expectations

## Excluded

- `src/boom_all.cpp`
- divider files or DIV/REM implementation
- build, HLS project, XSim, and backup-log artifacts
