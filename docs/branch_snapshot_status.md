# Branch Snapshot Status

Date: 2026-07-24

## Conclusion

M004 and branch snapshots describe different issues.

| Question | Answer |
|---|---|
| Does M004 mean all branch recovery is fixed? | No. M004 only covers the concrete Gate 1 JALR redirect test. |
| Are BOOM branch snapshots implemented in HLS? | No. HLS has `br_snapshots` storage but does not allocate `br_tag`, set `br_mask`, save map/free-list snapshots, or restore them on mispredict. |
| Does HLS use a functional substitute? | Partially. Current HLS uses coarse redirect/flush plus committed-map recovery. This is enough for the serialized subset tests but is not BOOM's original branch recovery mechanism. |
| Architectural impact | PARTIALLY_VERIFIED. Gate 1 branch/JAL/JALR tests pass in the supported subset, but multiple unresolved branches and wrong-path physical allocation are not fully covered. |
| Microarchitectural impact | MISMATCH. BOOM's branch tags, masks, snapshots, allocation-list recovery, and selective younger-uop kill are absent. |
| Cycle impact | INSUFFICIENT_EVIDENCE. No BOOM Verilator event trace exists yet, and the HLS flush/recovery timing is not proven cycle-equivalent. |
| Should M004 be downgraded? | No. M004 remains VERIFIED as a specific JALR redirect mismatch. Branch snapshot recovery remains a separate mismatch. |
| Is a separate mismatch needed? | Yes. M009 remains the independent branch snapshot/free-list recovery mismatch. |

## Required Reporting Rule

Do not close M009 based on M004. M004 is a test-specific architectural closure; M009 is a structural and cycle-relevant BOOM mechanism mismatch.
