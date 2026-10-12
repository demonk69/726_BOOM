# Gate Status Semantics

## Historical Field Audit

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED` has historically represented the broader Gate6 L3 acceptance state, not only the latest full-core functional program result.

Evidence:

| Source | Historical Meaning |
| --- | --- |
| `reports/gate6_0_full_lsu/l3/l3_results.md` | Kept false while RTL/synthesis/PPA gates remained incomplete. |
| `reports/gate6_0_full_lsu/l3/rtl_verification/remaining_coverage_gaps.md` | Explicitly says not to flip the gate-wide flag from partial RTL evidence. |
| `reports/gate6_0_full_lsu/l3/rtl_verification/r4_reconciliation/transaction_stress_final.md` | Kept false even after five-config stress PASS because gate policy remained broader. |
| `reports/gate6_0_full_lsu/l3/ppa_feasibility/outstanding_capacity_review.md` | Kept false because PPA acceptance policy and placed/routed implementation were unresolved. |
| `reports/gate6_0_full_lsu/l3/full_core_signoff/remaining_gate_blockers.md` | Kept false while multi-outstanding full-core workload and board/XDC blockers remained open. |

## Status Taxonomy

| Status | L3R6 Value | Meaning |
| --- | --- | --- |
| L3 functional verification | PASS | Native/CSim and RTL stress history remain valid. |
| L3 full-core program verification | PASS | PPA6 full-core program reached max observed outstanding `3`, reordered responses, store forwarding, final regs/memory, and commit trace PASS. |
| L3 full-core capacity 4_4 | PASS | L3R6 capacity program naturally reached `4` external outstanding loads and continued after slot release. |
| L3 PPA verification | UNRESOLVED | PPA4 compact route is physical evidence but not board bitstream signoff. |
| Gate6 overall signoff | UNRESOLVED | Board XDC/bitstream and final PPA policy remain open. |

## Field Decision

Because the historical field includes PPA/full-gate semantics, L3R6 does not redefine it.

`L3_FULL_CORE_MULTI_OUTSTANDING_FUNCTIONAL_VERIFIED=true`

`L3_R6_GATE_SEMANTICS_AUDITED=PASS_HISTORICAL_GATE_FIELD_RETAINED`

`PPA_GATE_STATUS=UNRESOLVED`

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false`
