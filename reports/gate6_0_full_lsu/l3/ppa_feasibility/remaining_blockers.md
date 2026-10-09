# Remaining Blockers

## Status

| Field | Value |
|---|---|
| `L3R5_EVIDENCE_COMMIT` | `236733e80a1fd60851436c5b9fe066ff81b09ea7` |
| `L3_PRODUCT_CODE_MODIFIED` | `false` |
| `G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED` | `false` |
| `PPA_GATE_STATUS` | `UNRESOLVED` |

## Blockers

| ID | Blocker | Evidence | Required resolution |
|---|---|---|---|
| PPA-B1 | PPA acceptance policy is unresolved because the historical `266795` LUT blocker exceeds the current device LUT capacity of `230400`. | `ppa_threshold_provenance.md` | Define the operative L3 PPA pass/review/blocker thresholds for this target device. |
| PPA-B2 | Raw `CONFIG_4_4` RTL is not placeable because of top-level I/O overutilization. | `implementation_feasibility.md`, `logs/4_4_impl.log` | Add or choose an integration wrapper/pin strategy that reduces exposed package I/O before rerunning implementation. |
| PPA-B3 | Route and post-route timing are unavailable. | `implementation_feasibility.md` | Complete placement and routing after PPA-B2 is resolved. |
| PPA-B4 | HLS-only timing and resources cannot close physical PPA. | `five_config_csynth_matrix.csv`, `hls_timing_analysis.md` | Re-run implementation on the selected integrated top and collect post-route WNS/TNS/resource reports. |

## Non-Blocker Evidence

- All five CSynth rows complete with exit code `0`, no `ERROR:` lines, and no `CRITICAL WARNING:` lines.
- All five CSynth rows report `6.071 ns` estimated period against the L3 `6.450 ns` planned limit.
- All five CSynth rows remain below the L3 planned review threshold of `228375` LUT.
- BRAM_18K remains `16` and DSP remains `3` across the depth matrix.

## Stop Condition

Do not promote this package to accepted PPA signoff until PPA-B1 and PPA-B2 are resolved and a post-route report is available for the selected implementation top.
