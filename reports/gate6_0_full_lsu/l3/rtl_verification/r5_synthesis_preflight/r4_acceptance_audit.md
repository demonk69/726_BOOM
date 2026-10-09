# R4 Acceptance Audit

Scope: Gate 6.0 L3R2-R4 five-config RTL stress evidence, audited before R4 evidence commit and before R5 synthesis preflight.

Current repository HEAD during audit: `dce3de35d1e3c17b5bda215c641fcb3e4a86f8ed`.

R3 evidence commit: `dce3de35d1e3c17b5bda215c641fcb3e4a86f8ed`.

Final command audited:

```bash
G6_L3_R2_CONFIGS="4_4 8_8 16_16 4_16 16_4" G6_L3_R2_REUSE_RTL=1 bash scripts/gate6_0/run_l3_r2_extended_rtl_stress.sh
```

| Config | Final status | Final cases | PASS markers | FAIL markers | Requests | Responses | Max outstanding | Disallowed markers |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `4_4` | PASS | 8 | 12 | 0 | 17 | 16 | 4 | none |
| `8_8` | PASS | 8 | 12 | 0 | 21 | 20 | 8 | none |
| `16_16` | PASS | 8 | 12 | 0 | 29 | 28 | 16 | none |
| `4_16` | PASS | 8 | 12 | 0 | 17 | 16 | 4 | none |
| `16_4` | PASS | 8 | 12 | 0 | 29 | 28 | 16 | none |

`cases=8` is the testbench assertion-group counter. The 12 PASS markers are the detailed scenario evidence extracted into `transaction_stress_matrix.csv`.

Original scenario mapping:

| Original R2 scenario | R4 evidence marker |
| --- | --- |
| `MAX_LQ_SATURATION` | `outstanding_slot_saturation`, `max_saturation_reuse_stale` |
| `ARBITRARY_OUT_OF_ORDER_RESPONSE` | `arbitrary_out_of_order_first` |
| `LQ_RELEASE_IMMEDIATE_REUSE` | `lq_release_immediate_reuse` |
| `STALE_RESPONSE_REJECTION` | `stale_response_after_lq_reuse` |
| `TXID_WRAP_DUPLICATE_SKIP` | `txid_wrap_duplicate_skip` |
| `CONSECUTIVE_RESPONSE_HANDLING` | `response_backpressure_consecutive` |
| `RESET_WITH_PENDING_TRANSACTIONS` | `reset_with_pending` |

Reconciled R3 blockers:

| Blocker | R4 evidence marker | Result |
| --- | --- | --- |
| Request backpressure sampled too early | `request_backpressure` | PASS |
| Store forwarding counted store traffic as load traffic | `store_forwarding`, `store_forwarding_partial_negative` | PASS |
| Flush test used direct `state_global_flush` poke | `flush_with_pending` | PASS |

Required status fields:

```text
L3R2_R4_BACKPRESSURE=PASS
L3R2_R4_STORE_FORWARDING=PASS
L3R2_R4_FLUSH_RECOVERY=PASS
L3R2_R4_FULL_STRESS_STATUS=PASS
L3R2_R4_VERIFICATION_COMPLETE=true
G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false
```

Result: `L3R2_R4_ACCEPTANCE_AUDIT=PASS`.

## Clean Reproduction

Detached worktree: `/home/lab_726/opencode_tmp/g6_l3r2_r4_clean_repro_20261009_155117`.

Isolated report root: `/home/lab_726/opencode_tmp/g6_l3r2_r4_clean_repro_reports_20261009_155117`.

The detached worktree was created at the R3 evidence commit and only the reviewed R4 testbench and runner were copied into it. The reproduction command used the same five configs and `G6_L3_R2_REUSE_RTL=1`.

| Config | Reproduction status | RTL hash match | Max outstanding |
| --- | --- | --- | ---: |
| `4_4` | PASS | true | 4 |
| `8_8` | PASS | true | 8 |
| `16_16` | PASS | true | 16 |
| `4_16` | PASS | true | 4 |
| `16_4` | PASS | true | 16 |

Reproduction `extended_config_matrix.csv` hash: `463b4333078dd39581922ae4c5cd9bbfbacdb1013997d83a6663a7a373c29749`.

Result: `L3R2_R4_CLEAN_REPRODUCTION=PASS`.
