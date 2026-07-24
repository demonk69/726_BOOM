# Gate 1 Results

Date: 2026-07-24

Verdict: VERIFIED for M003, M004, and M006 in the currently implemented integer/control subset.

## Summary

| Check | Before Gate 1 | After Gate 1 |
|---|---:|---:|
| Original directed suite | 20/25 | 25/25 |
| Gate 1 regression suite | N/A | 13/13 |
| Vitis HLS csim observable suite | not rerun | 5/5 |
| Merged HLS C++ compile | not rerun | PASS |

## Closed Mismatches

| Mismatch | Status | Evidence |
|---|---|---|
| M003 BEQ not-taken | VERIFIED | `t10_nt_branch` now requires fall-through write `x3=99` and final write `x3=10`; directed suite passes. |
| M004 JALR redirect | VERIFIED | `t12_jalr` now forms `RESET_VECTOR+16` using AUIPC+ADDI and reaches the JALR target; directed suite passes. |
| M006 IMEM backpressure | VERIFIED | Frontend now increments `fetch_id` per request and accepts only matching outstanding responses; `t17_imem_backpressure` and stale-response regression pass. |

## Additional Gate 1 Fix

| Item | Status | Evidence |
|---|---|---|
| IQ grant over-issue to unimplemented lanes | VERIFIED | `issue_module` caps grants to `DISPATCH_WIDTH` implemented ALU lanes; IQ port-conflict regression passes. |

## Source Changes

- `src/frontend.cpp`: monotonic request IDs, stale-response rejection, redirect/flush pending-response cleanup.
- `src/rename.cpp`: source operands read from speculative `map_table`; committed map remains commit/recovery state.
- `src/issue.cpp`: issue grants capped to implemented ALU execute lanes.
- `tb/differential/directed_tests.cpp`: corrected/strengthened WAR, ROB full, BEQ not-taken, JALR, and delayed IMEM tests.
- `tb/differential/gate1_regression_tests.cpp`: new Gate 1 regression suite.
- `scripts/run_csim.tcl`: explicit `exit` added for repeatable batch Vitis HLS csim.

## Commands Run

```sh
g++ -std=c++11 -I/home/lab_726/boom/hls_boom/include /home/lab_726/boom/hls_boom/tb/differential/directed_tests.cpp ... -o /tmp/boom_gate1_directed_final
/tmp/boom_gate1_directed_final
```

```sh
g++ -std=c++11 -I/home/lab_726/boom/hls_boom/include /home/lab_726/boom/hls_boom/tb/differential/gate1_regression_tests.cpp ... -o /tmp/boom_gate1_regression_final
/tmp/boom_gate1_regression_final
```

```sh
/home/lab_726/boom/hls_boom/scripts/generate_merged.sh
g++ -std=c++11 -I/home/lab_726/boom/hls_boom/include -c /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp -o /tmp/boom_core_merged_gate1.o
```

```sh
FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls -f /home/lab_726/boom/hls_boom/scripts/run_csim.tcl
```

## Remaining Blocking Items

- Cycle Equivalence remains INSUFFICIENT_EVIDENCE because no BOOM Verilator reference trace exists yet.
- Branch snapshots/free-list allocation-list recovery remain MISMATCH for full BOOM behavior.
- Full BOOM modules remain NOT_IMPLEMENTED: LSU, ICache, DCache, MMU/Sv39, TLB, FPU, branch predictor, TileLink, L2.
- HLS timing remains at the previous 92.13 MHz evidence; no Gate 1 timing optimization was attempted.

## Gate Decision

Gate 1 passed for the supported subset. Gate 2 may start next, but Cycle Equivalence must not be claimed until real BOOM Verilator traces are generated and compared.
