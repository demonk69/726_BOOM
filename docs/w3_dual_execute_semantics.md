# W3 Dual Execute Semantics

This document records the current source semantics of the W3 issue, execute, and completion path. It is a behavioral description, not a final verification or PPA claim.

## Width and lane mapping

The configured interface has `ISSUE_WIDTH == 3`, but only two integer-side lanes participate:

| Lane | Constant | Accepted class | Current role |
|---|---|---|---|
| 0 | `MEM_ISSUE_LANE` | Supported load/store uops | MEM issue, execute, and completion slot |
| 1 | `INT_ISSUE_LANE` | Supported ALU, branch, jump, and multiply uops | INT issue, execute, and completion slot |
| 2 | `FP_ISSUE_LANE` | None | Reserved; `port_ready` is false and execute clears its result slot |

Classification fixes a supported uop to MEM or INT. The implementation does not dynamically steer one class to the other lane.

## Independent acceptance

The issue queue scans for the first ready MEM candidate and the first ready INT candidate. It can generate both grants in one call. Each grant is accepted independently using only its corresponding `port_ready` plus lane-specific LSU capacity checks:

- A held MEM completion or a full required LDQ/STQ resource blocks the MEM grant without blocking INT.
- A held INT completion blocks the INT grant without blocking MEM.
- An accepted queue entry is marked granted and removed by IQ compaction; an unaccepted entry remains requestable.
- A single persistent dispatch packet may use its classified lane directly if that lane has no existing grant and the packet can otherwise be preserved. The direct path does not create a second dispatch input.

Issue recomputes source readiness from the current integer busy table. It does not rely only on busy bits captured earlier in the uop.

## Persistent completion slots

`ExecuteState::alu_results[0]` and `[1]` are one-entry persistent completion slots for MEM and INT respectively.

- Execute writes a lane's slot only when that lane issued a uop and its slot is invalid.
- Execute never overwrites a valid slot.
- A valid slot deasserts `port_ready` only for the corresponding lane on the next issue decision.
- A completion blocked by LDQ or STQ admission remains intact for retry.
- Stale slots whose ROB index or allocation identity no longer matches a live ROB entry are discarded by completion service, releasing that lane.

There is no deeper per-lane completion FIFO. The two slots are storage for one pending result per active lane.

## Completion service and writeback

`rob_complete` services at most one execute completion per core cycle. It first removes stale identity mismatches, then selects the valid matching completion with the smallest wrap-safe age from the current ROB head. Physical lane number is not the arbitration priority.

Completion is conservative:

- Execute computes results but does not write the integer register file or clear destination busy state.
- Before any completion side effect, the implementation requires a live ROB entry with matching `rob_idx` and `rob_allocation_id`.
- A non-load integer result writes the physical register file and clears its busy bit only when selected by `rob_complete`.
- A load completion first enters the LDQ and remains ROB-busy. Its data writes back only on a matching memory response.
- A store completion must enter the STQ before its ROB entry becomes non-busy.
- Full LDQ/STQ admission leaves the selected MEM completion held and does not partially update the ROB.

If a DMEM response is present at the start of `boom_core_step`, the normal execute-completion service is skipped for that cycle. LSU response handling owns the single writeback opportunity. A response writes a load destination only when transaction ID, pending ROB index, allocation ID, live ROB entry, and the entry's recorded transaction ID agree.

Commit remains one-wide (`COMMIT_WIDTH == 1`) and is separate from execute completion service.

## ROB and memory identity

ROB allocation assigns each accepted dynamic uop both a wrapping physical `rob_idx` and a nonzero 32-bit `rob_allocation_id`. The allocation ID is copied through dispatch, IQ, issue, execute, LSU entries, and the ROB. It distinguishes different occupants of the same wrapping ROB index.

`next_allocation_id` advances once per new allocation and skips zero. Retry does not consume another ID, and runtime reset does not rewind the counter.

Loads and committed stores share `LsuState::next_transaction_id`. Each emitted DMEM request takes the current value and increments the 32-bit counter. Runtime reset clears pending LSU request identity but deliberately does not rewind `next_transaction_id`; IDs are therefore monotonic modulo their fixed width across ordinary operation and reset. This is not an unbounded uniqueness guarantee after 32-bit wraparound.

## Branch and reset priority

A selected branch completion invokes branch resolution before its ROB busy bit is cleared or its ordinary result is written back. On a mispredict, recovery kills matching younger dispatch, IQ, issued, execute-completion, LSU, and ROB state, restores rename state, and redirects the frontend. A younger completion killed during that recovery cannot subsequently write back. Since completion arbitration is by ROB age, an older valid completion is serviced before a younger branch even if the branch is in the other lane.

At the top-level cycle boundary, incomplete reset sequencing has priority over all normal core modules. No issue, execute, completion, LSU, or commit call runs during that reset step. Reset clears both active completion slots, the persistent dispatch packet, issue state, ROB contents, and pending LSU state over its reset phases while preserving the monotonic identity counters.

## Explicit boundaries

- Rename, decode, dispatch, ROB allocation, and commit remain one-wide.
- Dual acceptance is limited to one fixed MEM lane and one fixed INT lane.
- There is one completion slot per active lane and only one execute-completion service per cycle.
- The FP lane and full FP execution are not part of this path.
- This is not W4 and is not a full wide-issue, wide-writeback, or wide-commit implementation.
- This document does not assign final `VERIFIED` status and does not report test or synthesis totals.
