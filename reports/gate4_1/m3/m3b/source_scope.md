# M3B Source Scope

Canonical implementation inputs are the modular files under `include/` and `src/`, regenerated into `src/boom_core_merged.cpp` by `scripts/generate_merged.sh`.

Expected implementation scope:

- `include/boom_state.hpp`
- `src/issue.cpp`
- `src/execute.cpp`
- `src/branch.cpp`
- `src/reset.cpp`
- generated `src/boom_core_merged.cpp`
- M3B tests, RTL harnesses, scripts, documentation, and reports

Explicit exclusions:

- `src/boom_all.cpp`
- frontend work
- full LSU work
- cache, MMU, and FPU work
- M3C PPA directive experiments
- `CORE_CYCLE` pipelining, `DATAFLOW`, false-dependence, and completion array-partition directives
