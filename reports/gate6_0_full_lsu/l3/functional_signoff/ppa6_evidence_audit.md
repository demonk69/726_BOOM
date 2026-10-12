# PPA6 Evidence Audit

PPA6 evidence commit: `244127b18abecd1364b61700bb03a8afb5777acd`.

Scope audited: `reports/gate6_0_full_lsu/l3/full_core_end_to_end/` plus generated run output `/home/lab_726/opencode_tmp/g6_l3r5_ppa6_full_core_e2e_20261010`.

## Evidence Inventory

| Artifact | Status | Notes |
| --- | --- | --- |
| Program source | PASS | `ppa6_multi_outstanding_program.S`; normal RV64 program loaded through IMEM. |
| Testbench | PASS | `ppa6_multi_outstanding_tb.sv`; instantiates `boom_core_top` and AXIS IMEM/DMEM models. |
| Runner | PASS | `ppa6_run_full_core_e2e.sh`; rebuilds program, compiles generated RTL, runs xsim, then runs analyzer. |
| Analyzer | PASS | `ppa6_analyze_full_core_e2e.py`; computes static expected architectural state and checks trace artifacts. |
| Simulation log | PASS | `ppa6_multi_outstanding.stdout.log` ends with `PPA6_FULL_CORE_E2E,COMPLETE`. |
| Transaction trace | PASS | `outstanding_transaction_trace.csv` records load request and response handshakes with txids. |
| Commit trace | PASS | `commit_reference_comparison.csv` and `full_core_execution_trace.csv` show reset-to-entry and ordered commit sequence. |
| Register comparison | PASS | `final_register_comparison.csv` all rows PASS. |
| Memory comparison | PASS | `final_memory_image_comparison.csv` all rows PASS. |
| RTL source | PASS | Same generated `boom_core_top` as PPA5 B3I and PPA4 compact route core instance. |

## Audit Checks

| Check | Result | Evidence |
| --- | --- | --- |
| RTL from canonical product source | PASS | `rtl_provenance_comparison.md`: product source SHA `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7`, RTL top SHA `9adbf00aa02cc89def9f73d34f860e2865bef0c1771106f94bce97f6e5459659`. |
| No internal ROB/LQ/completion injection | PASS | Testbench only observes `dut.state_lsu_outstanding_load_count_V`; no assignments, `force`, or state writes into DUT internals. |
| Three outstanding are real external handshakes | PASS | `ppa6_sim_summary.csv`: `max_external_outstanding=3`; `outstanding_transaction_trace.csv` has accepted loads with external outstanding count reaching 3. |
| Reordered responses changed completion order | PASS | Response txid `0x3` request order 2 returns before txid `0x2` request order 1; `response_reorder_count=2`. |
| Load data matches transaction owner | PASS | Analyzer maps each response txid back to accepted address/data; sensitivity test detects swapped response data. |
| ROB commit order matches reference | PASS | `commit_reference_comparison.csv`: 28/28 PC subsequence rows PASS. |
| Final architectural state from independent reference | PASS | Expected registers/memory are static program-oracle values, not read from DUT internals. |
| Store-forwarding completion value correct | PASS | Same-address load commits `x20=0x5a`; `store_forward_external_loads=0`, `store_forward_commits=1`. |
| Tohost entered commit path | PASS | `tohost_store_seen=1`, `tohost_commit_seen=1`, final memory `0x80000080=1`. |
| Hidden FAIL/SKIP/XFAIL | PASS | Run summary `fail_count=0`, protocol errors `0`; report FAIL strings are analyzer failure branches or intentional oracle-sensitivity negative cases only. |

## PPA6 Result

`L3_PPA6_FULL_CORE_END_TO_END_VERIFIED=PASS`

`L3_FULL_CORE_MULTI_OUTSTANDING_FUNCTIONAL_VERIFIED=true`

`L3_R6_FULL_CORE_FUNCTIONAL=PASS`

`PPA_GATE_STATUS=UNRESOLVED`

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false`
