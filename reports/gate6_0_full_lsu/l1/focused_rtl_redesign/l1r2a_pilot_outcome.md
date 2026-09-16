# L1R2A Stateless Focused RTL Pilot Outcome

## Result

```text
L1R2A_FEASIBILITY_PILOT=PASS
L1_GATE_STATUS=BLOCKED_17_OF_80
REPRESENTATIVE_SCENARIOS=6_OF_6
NEW_STATELESS_RTL_IMAGES=5
REUSED_CURRENT_SOURCE_PATH_B_SCENARIOS=1
PRODUCT_SOURCE_CHANGED=false
EXPANSION_TO_80_STARTED=false
```

P1 uses already accepted current-source asymmetric full-core RTL and was not rerun. P2 through P6 use five independently synthesized images with automatic local state and external SV oracles.

| Scenario | Path | Result | Evidence |
|---|---|---|---|
| P1 independent reset bounds | PATH_B canonical full core, 4/16 and 16/4 | REUSED PASS | `../asymmetric_reset_rtl_matrix.csv` |
| P2 LQ generation reuse | PATH_A `lsu_accept_completion` plus `lsu_finish_load_response` | RTL PASS | `logs/lq_reuse_xsim.log` |
| P3 stale response after slot reuse | PATH_A allocation, canonical global flush, reallocation, response classification | RTL PASS | `logs/stale_response_xsim.log` |
| P4 mispredict squash and pending-owner invalidation | PATH_A `branch_complete_event` | RTL PASS | `logs/branch_squash_xsim.log` |
| P5 older-store blocks younger load | PATH_A exact `older_store_in_rob` product predicate | RTL PASS | `logs/older_store_block_xsim.log` |
| P6 SQ stable identity and stale token rejection | PATH_A SQ accept/match/reclaim functions | RTL PASS | `logs/sq_identity_xsim.log` |

## Resource Policy

All new accepted images stayed below 4 GiB soft and 8 GiB hard limits and below the 900 s timeout. The accepted-image maxima were 304,964 KiB workspace, 2,565,592 KiB aggregate RSS, and 296 s HLS runtime. Exact rows are in `pilot_resources.csv`.

Two non-accepted probes were terminated and are not counted as pilot PASS:

- Reset wrapper: 137,272 KiB workspace, 8,821,180 KiB RSS, 135 s, exit 143.
- Composite `lsu_module` older-store wrapper: 8,896,420 KiB workspace, 1,956,928 KiB RSS, 141 s, exit 143.

These failures establish the PATH_A/PATH_B boundary and are not requests for larger limits.

## Existing 17 Cases

The retained evidence states only `17/17` for an LSU reset-flush/backpressure subset. No per-case name, ID, filter, or assertion manifest survives. Therefore exact mapping to catalog IDs 0–79 is `UNMAPPED_ACCEPTED_AGGREGATE`; assigning specific IDs would fabricate evidence. The aggregate remains accepted but does not mark any individual partition row PASS.

The new pilot results are architecture-feasibility evidence and are not added arithmetically to 17: exact overlap with the unmapped aggregate cannot be excluded. Gate status therefore remains blocked at the retained `17/80` until expansion produces one unambiguous result row per catalog ID.

## Integrity

- Product input aggregate remains `b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618`.
- `src/boom_core_merged.cpp` remains `76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c`.
- `src/boom_all.cpp` remains `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- No 80-case expansion, T0R, or L2 work was started.
