# Outstanding Capacity Review

## Evidence Source

R4 RTL stress evidence is reused for observed outstanding-load capacity. The evidence was committed before the R5 synthesis preflight and reproduced from a clean checkout.

| Item | Value |
|---|---|
| R4 evidence commit | `61f6d57ef54c7b9cb6e9cebd34e30e26c1a04637` |
| R5 evidence commit | `236733e80a1fd60851436c5b9fe066ff81b09ea7` |
| Clean R4 reproduction worktree | `/home/lab_726/opencode_tmp/g6_l3r2_r4_clean_repro_20261009_155117` |
| Clean R4 report root | `/home/lab_726/opencode_tmp/g6_l3r2_r4_clean_repro_reports_20261009_155117` |

## Observed Capacity

| Config | LQ | SQ | Cases | Requests | Responses | Max observed outstanding | Stress status |
|---|---:|---:|---:|---:|---:|---:|---|
| `4_4` | 4 | 4 | 8 | 17 | 16 | 4 | PASS |
| `8_8` | 8 | 8 | 8 | 21 | 20 | 8 | PASS |
| `16_16` | 16 | 16 | 8 | 29 | 28 | 16 | PASS |
| `4_16` | 4 | 16 | 8 | 17 | 16 | 4 | PASS |
| `16_4` | 16 | 4 | 8 | 29 | 28 | 16 | PASS |

The observed outstanding count is the maximum simultaneous in-flight load count seen in RTL transaction traces, not the total request count.

## Interpretation

The stress traces demonstrate that each generated depth variant can expose outstanding load concurrency up to the load-queue depth under the R4 stress scenario. This supports the PPA feasibility analysis because the five synthesized configurations correspond to the same depth matrix used by the RTL stress evidence.

## Acceptance Status

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false` for this PPA package. The R4 traces are valid evidence for observed RTL behavior, but the current package does not by itself close the L3 acceptance gate because it does not include a placed/routed implementation and because the PPA acceptance policy remains unresolved.
