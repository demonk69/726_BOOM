# Full-Core Commit Trace Audit

PPA5 ran a current-R5 generated RTL program-level workload using the existing full-core RTL harness and memory/trace models.

## Run Identity

| Item | Value |
| --- | --- |
| Runner | `reports/gate6_0_full_lsu/l3/full_core_signoff/ppa5_b3i_current_rtl_sim.sh` |
| Output | `/home/lab_726/opencode_tmp/g6_l3r5_ppa5_b3i_current_rtl_20261010` |
| Generated RTL top | `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog/boom_core_top.v` |
| RTL top SHA-256 | `9adbf00aa02cc89def9f73d34f860e2865bef0c1771106f94bce97f6e5459659` |
| Workload matrix SHA-256 | `1d8e4eb959a7144f2304b1b6f36fbe7508177520d63797dc94f88c2b4a92c584` |
| Runner SHA-256 | `8b75654bc8027820e2e9e1ddf9f9df83c75e19e4fde95ebc83e302dba6a59180` |

## Results

| Program | Status | IMEM requests | DMEM requests | Commits | Architectural check |
| --- | --- | ---: | ---: | ---: | --- |
| `packet_two_rvc` | PASS | 14 | 1 | 11 | x8=12, x9=21, committed tohost=1 |
| `packet_rvc_rvc_long` | PASS | 14 | 1 | 11 | x8=3, x9=4, x10=7, committed tohost=1 |
| `packet_carry_plus_rvc` | PASS | 15 | 1 | 12 | x8=6, x9=17, x10=18, committed tohost=1 |
| `packet_atomic_backpressure` | PASS | 38 | 2 | 54 | x8=40, x9=19, x10=59, load result observed, committed tohost=1 |
| `packet_redirect` | PASS | 15 | 1 | 13 | x8=3, x9=7, x10=10, skipped younger compressed instructions, committed tohost=1 |
| `packet_fault` | PASS | 9 | 0 | 5 | illegal compressed instruction exception at `0x80000002`, cause `2`, no tohost |

## Trace Caveats

This is real full-core program execution, not adapter-only testing. It uses normal fetch from reset vector to `0x80000000`, IMEM responses from program hex, DMEM model responses, commit trace monitoring, and architectural signature checks.

It is still not the complete PPA5 workload because the B3I program set does not create multiple independent outstanding loads or out-of-order external responses. The prior L3 R4 stress evidence reaches max outstanding `4` for `CONFIG_4_4`, but that stressbench directly seeds internal ROB/LQ state and is not accepted as the PPA5 end-to-end program workload.

## Status Fields

`L3_PPA5_FULL_CORE_WORKLOAD=PARTIAL_PASS_B3I_PROGRAMS_MULTI_OUTSTANDING_PROGRAM_PENDING`

`L3_PPA5_FULL_CORE_LOAD_STORE=PARTIAL_PASS_SINGLE_LOAD_STORE_PROGRAM_PLUS_R4_PROTOCOL_STRESS`

`L3_PPA5_FULL_CORE_RVC=PASS`

`L3_PPA5_FULL_CORE_COMMIT_TRACE=PASS_FOR_B3I_PROGRAMS`

`L3_PPA5_MAX_OBSERVED_OUTSTANDING=4_R4_STRESS_NOT_PPA5_PROGRAM_WORKLOAD`
