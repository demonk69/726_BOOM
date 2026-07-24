# Verilator Trace Signal Inventory

Date: 2026-07-24

Scope: `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig` generated BOOM SmallBoomConfig artifacts.

## Existing Trace Mechanisms

| Mechanism | Status | Evidence |
|---|---|---|
| BOOM commit log printf | Disabled | `chipyard.TestHarness.SmallBoomConfig.anno.json:1234` has `enableCommitLogPrintf:false`. |
| BOOM branch printf | Disabled | `chipyard.TestHarness.SmallBoomConfig.anno.json:1235` has `enableBranchPrintf:false`. |
| BOOM memtrace printf | Disabled | `chipyard.TestHarness.SmallBoomConfig.anno.json:1236` has `enableMemtracePrintf:false`. |
| Tile trace port | Disabled | `chipyard.TestHarness.SmallBoomConfig.anno.json:1106` has `trace:false`. |
| Verilator VCD/FST tracing | Disabled in generated model | `VTestHarness_classes.mk:16-17` has `VM_TRACE=0`. |
| Top-level trace port | Not present | `VTestHarness.h:33-35` exposes only `clock`, `reset`, and `io_success`. |

## Candidate Internal Signals

These are generated Verilator internal members. They are usable only for this generated model and are not a stable public API without a rebuild using explicit public/dontTouch exposure.

| Event | Signal | File/Line | Width | Top Visible | Needs Public Annotation | Stable For Diff | Notes |
|---|---|---:|---:|---|---|---|---|
| ROB allocate | `rob_io_enq_valids_0` | `VTestHarness.h:8769` | 1 | No | Yes for robust flow | PARTIAL | Internal generated name. |
| ROB allocate index | `rob_io_enq_uops_0_rob_idx` | `VTestHarness.h:8770` | 5 | No | Yes | PARTIAL | Used with allocate generation, not globally unique alone. |
| Physical destination | `rob_io_enq_uops_0_pdst` | `VTestHarness.h:8771` | 6 | No | Yes | PARTIAL | Rename/ROB enqueue candidate. |
| Stale physical destination | `rob_io_enq_uops_0_stale_pdst` | `VTestHarness.h:8772` | 6 | No | Yes | PARTIAL | Useful for stale-pdst recovery. |
| Commit valid | `rob_io_commit_valids_0` | `VTestHarness.h:8778` | 1 | No | Yes | PARTIAL | Includes non-architectural conditions. |
| Architectural commit valid | `rob_io_commit_arch_valids_0` | `VTestHarness.h:8779` | 1 | No | Yes | PARTIAL | Best commit-valid candidate. |
| Commit PC | `csr_io_pc` | `VTestHarness.h:21551` | 40 | No | Yes | PARTIAL | Must be timing-aligned to retire valid. |
| Commit logical dst | `rob_io_commit_uops_0_ldst` | `VTestHarness.h:8787` | 6 | No | Yes | PARTIAL | Logical destination. |
| Commit dst type | `rob_io_commit_uops_0_dst_rtype` | `VTestHarness.h:8789` | 2 | No | Yes | PARTIAL | Used for `rd_valid`. |
| Commit pdst | `rob_io_commit_uops_0_pdst` | `VTestHarness.h:8783` | 6 | No | Yes | PARTIAL | No stable commit rd_value exposed. |
| Exception valid | `rob_io_com_xcpt_valid` | `VTestHarness.h:8793` | 1 | No | Yes | PARTIAL | Exception event candidate. |
| Flush valid | `rob_io_flush_valid` | `VTestHarness.h:8794` | 1 | No | Yes | PARTIAL | Flush event candidate. |
| Branch resolve | `brinfos_0_valid` | `VTestHarness.h:8808` | 1 | No | Yes | PARTIAL | Branch event candidate. |
| Branch mispredict | `brinfos_0_mispredict` | `VTestHarness.h:8809` | 1 | No | Yes | PARTIAL | Branch event candidate. |
| Branch taken | `brinfos_0_taken` | `VTestHarness.h:8810` | 1 | No | Yes | PARTIAL | Branch event candidate. |
| Redirect PC | `io_ifu_redirect_pc_REG` | `VTestHarness.h:21555` | 40 | No | Yes | PARTIAL | Redirect event candidate. |
| Execute request | `csr_exe_unit__DOT__alu_io_req_valid` | `VTestHarness.h:8892` | 1 | No | Yes | PARTIAL | Integer/CSR ALU path candidate. |
| Execute response | `csr_exe_unit__DOT__alu_io_resp_valid` | `VTestHarness.h:8893` | 1 | No | Yes | PARTIAL | Execute/writeback candidate. |
| Writeback valid | `ll_wbarb_io_out_valid` | `VTestHarness.h:8762` | 1 | No | Yes | PARTIAL | Long-latency writeback arbiter valid. |
| Writeback data | `ll_wbarb_io_out_bits_data` | `VTestHarness.h:21548` | 64 | No | Yes | PARTIAL | Not a commit-time architectural value. |

## Unavailable Or Insufficient

| Field | Status | Reason |
|---|---|---|
| Commit instruction | INSUFFICIENT_EVIDENCE | ROB debug instruction memory exists in generated RTL, but no stable `rob_io_commit_uops_0_debug_inst` or `rob_debug_inst_mem_R0_data_0` member is exposed in `VTestHarness.h`. |
| Commit rd_value | INSUFFICIENT_EVIDENCE | Writeback data is observable, but no stable commit-time architectural rd value is exposed. |
| Rename physical source map | INSUFFICIENT_EVIDENCE | Internal rename map signals are not exposed as stable public trace signals. |
| Fetch/decode full packets | INSUFFICIENT_EVIDENCE | Some internal fields exist, but no stable top-level trace interface. |

## Gate 2 Implication

Real traces require either rebuilding BOOM with explicit trace exposure or using current generated internals as a brittle prototype. This workspace lacks the original Chisel source tree, final simulator binary, `libfesvr`, `libdramsim`, and RISC-V toolchain, so Gate 2 cannot produce validated real BOOM traces here.
