# BOOM-HLS Equivalence Audit

Date: 2026-07-24

Scope: SmallBoomConfig HLS integer/control subset under Vitis HLS 2021.2.

## Gate 1 Verdict

Gate 1 is VERIFIED for M003, M004, and M006 within the currently implemented subset.

| Item | Status | Evidence |
|---|---|---|
| M003 BEQ not-taken | VERIFIED | Strengthened t10 requires fall-through x3=99 and final x3=10; directed suite passes. |
| M004 JALR | VERIFIED | t12 now forms RESET_VECTOR+16 with AUIPC+ADDI and reaches the target; directed suite passes. |
| M006 IMEM backpressure | VERIFIED | Frontend uses monotonic fetch IDs and drops stale responses; t17 and stale-response regression pass. |
| Directed suite | VERIFIED | 25 passed, 0 failed. |
| Gate 1 regressions | VERIFIED | 13 passed, 0 failed. |
| Vitis HLS csim | VERIFIED | Observable top-level testbench passes 5/5. |

## Current Equivalence Conclusions

| Dimension | Verdict |
|---|---|
| Architectural Equivalence | PARTIALLY_VERIFIED |
| Microarchitectural Equivalence | PARTIALLY_VERIFIED |
| Cycle Equivalence | INSUFFICIENT_EVIDENCE |
| Structural Correspondence | PARTIALLY_VERIFIED |

## Gate 2 Verdict

Gate 2 is PARTIAL PASS after Gate 2.5. The corrected standalone path evaluates the generated BOOM `VTestHarness` model and produces finite loadmem-backed traces that terminate through a real retired store to `tohost`. The workspace still lacks the final `simulator-chipyard-SmallBoomConfig` binary, original Chipyard git checkout, `libfesvr`, `libdramsim`, and a RISC-V ELF/binutils toolchain, so official Gate 2 and official noninterference remain blocked.

## Key Limitations

- Full BOOM equivalence cannot be claimed because Cache, MMU, FPU, LSU, TLB, branch predictor, TileLink, and L2 are absent or stubbed.
- Branch snapshots and free-list allocation-list recovery are still not implemented.
- M004 is only the Gate 1 JALR redirect closure; branch snapshots remain MISMATCH under M009.
- The HLS issue path is capped to the implemented single ALU execute lane; BOOM IssueWidth=3 is not structurally complete.
- Corrected standalone BOOM generated-model traces exist, but official Chipyard traces and HLS-vs-BOOM cycle comparison are still missing, so cycle equivalence remains INSUFFICIENT_EVIDENCE.

## Source Reference

- BOOM generated source: `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/`
- HLS source: `/home/lab_726/boom/hls_boom/`
- Gate 1 report: `/home/lab_726/boom/hls_boom/reports/equivalence/gate1_results.md`
- Gate 2 report: `/home/lab_726/boom/hls_boom/reports/equivalence/gate2_results.md`
- Gate 2.5 report: `/home/lab_726/boom/hls_boom/reports/equivalence/gate2_5_results.md`
