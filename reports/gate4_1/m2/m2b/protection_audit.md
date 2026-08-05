# M2B Protection Audit

Status: **PASS** for scope and topology protection.

- Production integration changes are limited to `src/execute.cpp`; `src/synth_module_tops.cpp` changes only the diagnostic execute wrapper.
- `src/completion.cpp` remains SHA-256 `78c50be9f2fea8396f1de8534f9a659e1190c91c7780ee68d4903c9188572de5`.
- `src/rob.cpp` remains SHA-256 `a73352b8db1939b6d3f34226307ef6e2a89f27de87c8fac87c8eeb0b1c9c2bf5`.
- `src/lsu.cpp` remains SHA-256 `c13765170c232f1168b4003c4e6215a4c92f6bdf613710219711ab404f2c53ff`.
- Excluded `src/boom_all.cpp` remains SHA-256 `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c` and is absent from M2B source bindings.
- The fixed topology remains three issue/result lanes, three completion pending slots, two integer PRF writes, three wakeups, three bypasses, and commit width one.
- Multiply completion uses only `COMPLETION_SOURCE_INT_EXECUTE`; no completion, writeback, wakeup, bypass, or ROB port was added.
- `src/mul.cpp` occurs exactly once in `src/boom_core_merged.cpp`.
- No divider implementation or DIV/REM execute case was added. Existing decode constants remain outside M2B execution scope.
- No DATAFLOW, false-dependence, complete array-partition, or CORE_CYCLE pipeline directive was added.
- No directive or PPA optimization was attempted after the 6.5 ns blocker was observed.
