# Gate 4.1 M3B Baseline Manifest

- Task-start Git HEAD: `10c0a1ee47fca7b71e51c5bbaf1a55f3ce687317`
- Entry state: `M2_MUL_FAMILY_VERIFIED=true`
- Entry state: `M3A_STANDALONE_DIVIDER_VERIFIED=true`
- Entry state: `READY_FOR_M3B_INT_DIVIDER_INTEGRATION=true`
- Canonical core cycle: `Pipelined=no`
- Tool: Vitis HLS 2021.2
- M2C accepted evidence: `reports/gate4_1/m2/m2c/`
- M3A accepted evidence: `reports/gate4_1/m3/m3a/`
- Completion sources: load response, MEM execute, INT execute
- PRF write ports: 2
- Wakeup ports: 3
- Bypass ports: 3
- ROB complete sources: 3
- Commit width: 1

M2C and M3A evidence trees are frozen and are not overwritten by M3B. The pre-existing modified `src/boom_all.cpp` remains excluded from canonical source, merge, tests, and synthesis.
