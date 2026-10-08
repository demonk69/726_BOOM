# Artifact Manifest

Durable repo artifacts:

`durable_artifact_hashes.csv` intentionally excludes itself from its checksum inventory.

| Path | Classification |
| --- | --- |
| `reports/gate6_0_full_lsu/l3/l3_architecture_audit.md` | durable report |
| `reports/gate6_0_full_lsu/l3/l3_outstanding_contract.md` | durable report |
| `reports/gate6_0_full_lsu/l3/l3_source_diff_scope.md` | durable report |
| `reports/gate6_0_full_lsu/l3/directed_test_matrix.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/exhaustive_summary.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/random_summary.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/csim_matrix.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/focused_rtl_case_catalog.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/focused_rtl_acceptance.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/l2_preservation_classification.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/preservation_matrix.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/parameter_matrix.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/ppa_matrix.csv` | durable report |
| `reports/gate6_0_full_lsu/l3/critical_path.md` | durable report |
| `reports/gate6_0_full_lsu/l3/directive_audit.md` | durable report |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/failure_inventory.md` | durable RVC triage report |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/failure_groups.csv` | durable RVC triage report |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/baseline_comparison.md` | durable RVC triage report |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/first_divergence.md` | durable RVC triage report |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/source_scope_audit.md` | durable RVC triage report |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/repair_outcome.md` | durable RVC triage report |
| `reports/gate6_0_full_lsu/l3/handoff_audit/canonical_source_snapshot_audit.md` | durable handoff audit report |
| `tb/differential/g6_l3_multi_outstanding_tests.cpp` | durable test source |
| `scripts/gate6_0/run_l3_multi_outstanding_native.sh` | durable test runner |
| `scripts/gate6_0/run_l3_multi_outstanding_csim.sh` | durable test runner |
| `scripts/gate6_0/l3_multi_outstanding_csim.tcl` | durable CSim flow |

Ephemeral build/log artifacts:

| Path | Classification |
| --- | --- |
| `/home/lab_726/opencode_tmp/g6_l3/native` | ephemeral native logs/binaries |
| `/home/lab_726/opencode_tmp/g6_l3/csim` | ephemeral Vitis CSim workspace/logs |
| `/home/lab_726/opencode_tmp/g6_l3/l2_native` | ephemeral L2 native preservation logs |
| `/home/lab_726/opencode_tmp/g6_l3/l2_csim` | ephemeral L2 CSim preservation logs |
| `/home/lab_726/opencode_tmp/g6_l3/preservation/rvc_report` | ephemeral failed RVC preservation logs |
| `/home/lab_726/opencode_tmp/g6_l3_rvc_current` | ephemeral focused current RVC rerun logs |
| `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline` | ephemeral clean archive baseline RVC rerun logs |

Final acceptance does not rely on ephemeral directories being durable; pass/fail summaries are captured in repo reports.
