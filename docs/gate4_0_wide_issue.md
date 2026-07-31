# Gate 4.0 Wide-Issue Boundary

Gate 4.0 starts from the current W3 fixed-lane implementation. The checked-in source does not yet implement W4 or full wide issue. This document defines the present boundary without assigning a final gate status.

## Current baseline

The current datapath can issue and accept up to two supported uops in one cycle only when they occupy different fixed classes:

- One MEM uop on `MEM_ISSUE_LANE` (lane 0).
- One INT uop on `INT_ISSUE_LANE` (lane 1).

The two lanes have independent readiness and one persistent execute-completion slot each. A blocked lane retains its IQ entry or completion without preventing the other lane from accepting. The FP lane (lane 2) is reserved and unavailable.

Despite that dual-lane behavior, the surrounding machine remains narrow:

| Function | Current width or behavior |
|---|---|
| Decode | One uop (`DECODE_WIDTH == 1`) |
| Rename/dispatch packet | One persistent packet (`DISPATCH_WIDTH == 1`) |
| ROB allocation | At most one packet per current cycle |
| IQ grant | At most one MEM plus one INT |
| Execute result storage | One persistent slot per active lane |
| Execute completion service | At most one slot per cycle, oldest by ROB age |
| Load response writeback | Owns the cycle's writeback opportunity when present |
| Commit | One ROB head entry (`COMMIT_WIDTH == 1`) |

## Ordering and identity baseline

Every ROB allocation receives a nonzero `rob_allocation_id` in addition to its wrapping ROB index. Completion and LSU paths require the allocation ID to match the live ROB occupant before side effects. ROB allocation IDs advance once per new packet, are stable across retries, and are not rewound by runtime reset.

DMEM loads and committed stores use a shared incrementing 32-bit transaction-ID source. Reset clears pending memory state without rewinding that source. Matching load responses require both transaction identity and ROB allocation identity, limiting stale-response aliasing across flush, ROB reuse, and reset. Counter monotonicity is modulo the fixed 32-bit width.

## Recovery and writeback baseline

Completion arbitration selects one matching result by wrap-safe ROB age. A branch selected for completion resolves before ordinary writeback; mispredict recovery removes matching younger state, including held completions, before those completions can update the register file. Reset sequencing has top-level priority and runs instead of a normal core cycle.

Writeback remains deliberately conservative. Execute only fills a completion slot. The selected completion performs non-load integer writeback after ROB identity validation; load data writes back only after a matching response. A present DMEM response suppresses normal execute-completion service for that cycle, so the implementation does not provide two simultaneous integer writeback services.

## What Gate 4.0 does not currently provide

- Two-wide decode, rename, or persistent dispatch.
- Two ROB allocations from two new dispatch packets in one cycle.
- Two MEM grants, two INT grants, or fungible steering across execution lanes.
- More than one pending completion per active lane.
- Two execute-completion services or general multiport register-file writeback in one cycle.
- Wide commit.
- Active FP issue/execute support.

Accordingly, the current source should be described as fixed MEM/INT dual issue and execute with serialized completion, not W4 and not full wide issue. Any future Gate 4.0 claim must distinguish expansion of frontend/rename/ROB width, lane steering, completion bandwidth, writeback ports, and commit width rather than inferring wide issue from two independent W3 grants.

No final `VERIFIED` designation, regression total, synthesis result, or acceptance decision is claimed here.
