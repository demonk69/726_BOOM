# Gate 5.4 PF1 Exception Recovery Foundation

Verdict: **PASS WITH PRE-EXISTING PRESERVATION GAP**.

PF1 implements precise ROB-head synchronous exception take and recovery to
`0x10100`. Illegal instruction, breakpoint/C.EBREAK, and instruction access
fault tests produce one exception commit, write machine trap CSRs, squash all
younger speculative work, redirect Fetch, execute a handler signature, and
continue to the existing success ECALL. Recoverable exceptions do not assert
the terminal `io_trap` output.

## Results

| Requirement | Result | Evidence |
|---|---:|---|
| Directed recovery | 1817/1817 PASS | `logs/exception_recovery_tests.log` |
| Persistent random | 256 seeds x 8192 cycles, 25,165,824 checks, zero errors | `logs/exception_recovery_random_tests.log` |
| Native handler programs | 8/8 PASS | `logs/exception_recovery_program_tests.log` |
| Vitis CSim handler programs | 8/8 PASS | `logs/pf1_exception_csim.log` |
| Focused generated RTL | 64/64 PASS | `logs/pf1_exception_rtl.log` |
| Generated full-core RTL | 8/8 PASS | `logs/pf1_full_core_rtl_0.log` through `_7.log` |
| Canonical csynth | 8/8 PASS | `../../gate5_4_pf1_final2/module_csynth_summary.csv` |
| W3 preservation | 400/400 PASS | `/tmp/boom_hls/pf1/preservation/w3_final3_report/regression_after.md` |
| RV64M preservation | directed/random and 15/15 programs PASS | `/tmp/boom_hls/pf1/preservation/m3c_final_report/logs/` |
| RVC full-core native | 11/11 PASS | current-source run |
| W4 multi-writeback | 13/13 PASS | current-source run |

The accepted Gate 5.3 Fetch Buffer focused suite passes 169/169. Its persistent
random suite currently reports `drop=425388` and `ordering_error=174170` both
with PF1 and with a baseline-equivalent build that removes the PF1
`BoomCoreState` field and Frontend redirect changes. It is therefore not a PF1
regression, but remains an open baseline preservation gap.

No Predictor, FTQ, LSU expansion, FPU, or backend width/topology change is part
of PF1. `src/boom_all.cpp` is excluded from all PF1 compilation and evidence.
