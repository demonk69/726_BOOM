# PF6 Current-Source Run Status

- Product/header aggregate SHA-256: `7fa60c73d7c2a584ecb859e8d4f6ec5270ad5d8d6da9751030af6becdc68d808`.
- Existing current-source `boom_core_pf4_rtl_top` synthesis: PASS.
- XSim compile and elaboration after generated hierarchy synchronization: PASS.
- Progress root cause: stale testbench cycle-observer state caused a permanent
  wait before `start_fetch`; simulation time itself advanced normally.
- Harness-only repair: generated wait state 34 was updated to current
  equivalent state 35. Product source and RTL were unchanged.
- Product programs: 12/12 PASS.
- Mandatory predicted-taken fault checks: 2/2 PASS.

`PF6_CURRENT_SOURCE_FULL_CORE_RTL=PASS_12_OF_12`

`PF6_MANDATORY_FAULTS=PASS_2_OF_2`
