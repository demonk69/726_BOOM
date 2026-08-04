# M2A Protection Audit

Status: **PASS**.

- `src/execute.cpp`: unchanged, SHA-256 `70a5682b8c97318c3bfefd63ebd9ad3053544cfc74c57edee178d58edf552649`.
- `src/completion.cpp`: unchanged, SHA-256 `78c50be9f2fea8396f1de8534f9a659e1190c91c7780ee68d4903c9188572de5`.
- `src/rob.cpp`: unchanged, SHA-256 `a73352b8db1939b6d3f34226307ef6e2a89f27de87c8fac87c8eeb0b1c9c2bf5`.
- `src/lsu.cpp`: unchanged, SHA-256 `c13765170c232f1168b4003c4e6215a4c92f6bdf613710219711ab404f2c53ff`.
- `src/boom_all.cpp`: retained its task-start hash `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`; it is absent from source manifests and generated merged source.
- `src/mul.cpp` appears exactly once in the generated merged source.
- No divider header/source exists.
- No full-core RTL or full-core csynth was run.
- No completion, PRF, ROB, LSU, branch, reset, or `CORE_CYCLE` directive changed.
- Existing trace expectation files were not modified.
