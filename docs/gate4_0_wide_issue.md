# Gate 4.0 Wide-Issue Boundary

Gate 4.0 uses the source-bound W4E fixed-lane implementation as its canonical final configuration. It adds two physical integer PRF writes to W4C multi-wakeup/bypass, but it does not implement full wide issue or wide commit.

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
| Execute completion service | Up to three validated results may wake; up to two distinct destinations write PRF |
| Load response writeback | Participates in the age-ordered two-write arbitration |
| Commit | One ROB head entry (`COMMIT_WIDTH == 1`) |

## Ordering and identity baseline

Every ROB allocation receives a nonzero `rob_allocation_id` in addition to its wrapping ROB index. Completion and LSU paths require the allocation ID to match the live ROB occupant before side effects. ROB allocation IDs advance once per new packet, are stable across retries, and are not rewound by runtime reset.

DMEM loads and committed stores use a shared incrementing 32-bit transaction-ID source. Reset clears pending memory state without rewinding that source. Matching load responses require both transaction identity and ROB allocation identity, limiting stale-response aliasing across flush, ROB reuse, and reset. Counter monotonicity is modulo the fixed 32-bit width.

## Recovery and writeback baseline

Completion arbitration selects results by wrap-safe ROB age and fixed source priority. Up to three validated retained values publish fixed wakeup/bypass snapshots; the busy table is cleared only when one of two selected writes occurs or a same-value write is safely coalesced. A branch selected for completion resolves before ordinary writeback; mispredict recovery removes matching younger pending and transient forwarding state before issue. Reset sequencing has top-level priority and runs instead of a normal core cycle.

Execute only fills a completion slot. Up to two selected completions perform integer writeback after ROB identity validation; load data writes back only after a matching response. A third writer is retained. Same-destination/different-value events fail-stop with explicit conflict state and no write.

## What Gate 4.0 does not currently provide

- Two-wide decode, rename, or persistent dispatch.
- Two ROB allocations from two new dispatch packets in one cycle.
- Two MEM grants, two INT grants, or fungible steering across execution lanes.
- More than one pending completion per active lane.
- Wide commit.
- Active FP issue/execute support.

Accordingly, the current source is fixed MEM/INT dual issue and execute with source-bound W4E two-port integer writeback, not full wide issue. Any wider claim must distinguish frontend/rename/ROB width, lane steering, completion bandwidth, writeback ports, and commit width.

W4D phase evidence is recorded in `reports/gate4_0/w4/w4d_stage_results.md`. Waiver-free W4E software/random, focused RTL, 49-case full-core comparisons, and seven-target final csynth pass. The final two-bank LVT PRF closes W4 multiwrite, but decode, dispatch, and commit remain width one, so this is still fixed MEM/INT partial-width execution rather than full wide issue.

`READY_FOR_FRONTEND_IMPLEMENTATION=false`: decode, dispatch, and commit are still width one and strict BOOM cycle equivalence is absent; the W4 directive guardrail itself is closed. `READY_FOR_FULL_LSU_IMPLEMENTATION=false`: the current LSU is intentionally minimal and lacks cache/MMU/TLB/PTW/replay/ordering prerequisites.
