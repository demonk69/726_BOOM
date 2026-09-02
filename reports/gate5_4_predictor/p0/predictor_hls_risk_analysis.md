# Predictor HLS Risk Analysis

## Vitis HLS 2021.2 Risks

- Table read: a combinational same-cycle read may become a large mux for 64-512 counters; fixed one-cycle lookup gives HLS a clean state boundary without a core pipeline directive.
- Table write: commit-qualified BIM update is one write per cycle. Preserve an explicit valid/token when update backpressures.
- Same-cycle read/write: foundation requires write-forwarding. If commit update and lookup indices match, lookup observes the newly updated saturating counter through an explicit comparator/bypass, independent of inferred RAM mode.
- Update conflict: one lookup plus one commit write may require 1R1W behavior. If the inferred memory cannot provide it, serialize or bypass; do not duplicate tables silently.
- Memory inference: 128-1024 bits may map to registers/LUTRAM; BRAM can waste capacity and add width/latency constraints. Start with `AUTO`, measure standalone, then approve mapping.
- Reset: do not reset counter payload/table. Reset a separate per-entry valid bitmap as control state; resetting every counter risks reset fanout and blocks RAM inference.
- Initialization: an invalid entry reads logically as weak-not-taken. Its first committed update applies to that logical value, writes the counter, and sets that entry's valid bit. There is no background payload initialization and no insufficient global-valid shortcut.
- Multiple packet lanes: select the earliest CFI first, then perform one BIM access. Do not partition the whole table to support two speculative lane reads.
- Branch writeback: Execute correction and commit training are separate; store actual result/update token until commit.
- Generation: stale responses and stale commit updates must be dropped before table writes.

## Candidate Mapping

| Mapping | Recommendation |
|---|---|
| `AUTO` | foundation default; collect standalone inferred-storage and timing evidence |
| `LUTRAM` | candidate if AUTO creates excessive FF/LUT muxing; only after measured comparison |
| `BRAM` | unlikely efficient for small BIM; reconsider for larger/combined tables |

No directive is authorized by P0. Payload/table full reset, `DATAFLOW`, false `DEPENDENCE`, complete `ARRAY_PARTITION`, and `CORE_CYCLE` pipeline remain prohibited.
