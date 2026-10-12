# Remaining Gate6 Blockers

## Resolved By L3R6

| Item | Status | Evidence |
| --- | --- | --- |
| PPA6 full-core multi-outstanding program | RESOLVED_FUNCTIONAL | `full_core_end_to_end/validation_matrix.csv` all required checks PASS except separate exception recovery note. |
| Oracle independence and sensitivity | RESOLVED | `oracle_sensitivity.md` detects swapped load data, omitted response, and bad expected register. |
| CONFIG_4_4 natural capacity | RESOLVED_FUNCTIONAL | `full_core_capacity_matrix.csv`: max external/internal outstanding `4`, reordered responses `2`, post-capacity accepts `2`. |
| RTL provenance | RESOLVED_FOR_FUNCTIONAL_SCOPE | Same canonical source and generated core RTL as PPA5/PPA6 and PPA4 core instance. |

## Still Open

| Blocker | Status | Detail |
| --- | --- | --- |
| Board XDC | OPEN | No board definition, pin LOC, I/O standards, or timing model. |
| Bitstream signoff | OPEN | `write_bitstream` not rerun because `NSTD-1` and `UCIO-1` remain unresolved without board XDC. |
| Full-board PPA acceptance policy | OPEN | PPA4 compact route is useful physical evidence, but not a board-valid true-program PPA pass. |
| 8_8 and 16_16 program-level capacity | NOT_RUN | No current generated full-core RTL bundles for those configs in the PPA6 evidence scope. |
| Exception recovery in same multi-load program | OPEN_FOR_SINGLE_PROGRAM | PPA5 B3I `packet_fault` covers exception behavior separately. |

## Final Fields

`L3_R6_PPA6_EVIDENCE_COMMIT=244127b18abecd1364b61700bb03a8afb5777acd`

`L3_R6_FULL_CORE_FUNCTIONAL=PASS`

`L3_R6_ORACLE_INDEPENDENCE=PASS_STATIC_PROGRAM_ORACLE_NO_DUT_STATE`

`L3_R6_ORACLE_SENSITIVITY=PASS`

`L3_R6_MAX_OUTSTANDING_4_4=4`

`L3_R6_MAX_OUTSTANDING_8_8=NOT_RUN`

`L3_R6_MAX_OUTSTANDING_16_16=NOT_RUN`

`L3_R6_FULL_CORE_CAPACITY_4_4=PASS`

`L3_R6_FULL_CORE_CAPACITY_8_8=NOT_RUN_RESOURCE_SCOPE`

`L3_R6_FULL_CORE_CAPACITY_16_16=NOT_RUN_RESOURCE_SCOPE`

`L3_R6_RTL_PROVENANCE=PASS_SAME_CANONICAL_SOURCE_SAME_GENERATED_CORE_DIFFERENT_TOPS_AND_ADAPTERS`

`L3_R6_GATE_SEMANTICS_AUDITED=PASS_HISTORICAL_GATE_FIELD_RETAINED`

`L3_R6_PRODUCT_CODE_MODIFIED=false`

`PPA_GATE_STATUS=UNRESOLVED`

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false`
