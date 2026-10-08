# Gate 6.0 L3 Architecture Audit

Baseline commit: `2c6c5067274d9c310c637d20752bc271590bcded`

Branch: `gate3.8-rtl-verification`

Audit status: read-only audit completed before source edits.

## Current Single-Outstanding Limit

The current implementation enforces a global single external load outstanding through these mechanisms:

1. `LsuState::load_response_pending` is set on any non-forwarded load request and cleared only by `lsu_finish_load_response`, branch kill, flush, or reset.
2. `try_issue_load` returns early if `state.lsu.load_response_pending` is true.
3. `lsu_module` stops scanning the ROB when `state.lsu.load_response_pending` is true.
4. `completion_from_load_response` only accepts a response whose txid matches `pending_load_transaction_id` and whose ROB/LQ tuple matches the single global pending owner fields.
5. Branch squash and reset clear the same global pending owner fields.

The current global owner fields are `pending_load_transaction_id`, `pending_load_rob_idx`, `pending_load_allocation_id`, `pending_load_lq_index`, and `pending_load_lq_generation`.

## Transaction ID Generation

`state.lsu.next_transaction_id` is initialized to `1`. Current load issue assigns `tx = state.lsu.next_transaction_id++` only after `pipe.dmem_req.full()` is false. Store commit also uses `state.lsu.next_transaction_id++`. There is no current active-load collision check on wrap.

## Response Identity Fields

`DmemResponse` contains only:

1. `transaction_id`
2. `data`
3. `read_data`
4. `exception`
5. `exc_cause`
6. `exception_cause`

It does not carry LQ index, LQ generation, ROB index, or ROB allocation ID. Therefore stable owner identity must be recovered from the active transaction owner stored inside the core.

## LQ Entry Metadata Sufficiency

Current `LoadQueueEntry` already stores the minimum owner metadata needed for per-entry outstanding ownership:

1. `valid`
2. `response_pending`
3. `transaction_id`
4. `rob_idx`
5. `rob_allocation_id`
6. `generation`
7. `address`
8. `size`
9. `signed_load`
10. `branch_mask`

The ROB entry carries the destination PRF identity and canonical micro-op owner state. Completion can reconstruct value extension from LQ/ROB size and sign metadata.

## Per-LQ Ownership Feasibility

`L3_PER_LQ_OWNERSHIP_FEASIBLE=true`

Per-LQ-entry ownership is feasible. Each valid LQ entry can own at most one active external load transaction by using `LoadQueueEntry::response_pending` plus `transaction_id` as the active bit and txid, and the existing LQ generation, ROB index, and ROB allocation ID as stable identity. No second fixed-size MSHR table is required.

The theoretical maximum outstanding load count is therefore `LQ_DEPTH`: 4, 8, or 16 depending on configuration. SQ depth is not part of the load outstanding capacity.

## Channel Width Audit

The internal/external DMEM response interface is a single `hls::stream<DmemResponse>`. `boom_core_top` moves at most one response from `dmem_resp_in` into `pipe.dmem_resp` per cycle, and `completion_service_cycle` reads at most one response per cycle when the held load response slot is empty. L3 should preserve this single-response-per-cycle interface.

The DMEM request interface is a single `hls::stream<DmemRequest>`. `boom_core_top` drains at most one `pipe.dmem_req` entry to `dmem_req_out` per cycle. L3 should preserve at most one new external load request per cycle and must not add a multi-request issue path.

## Forwarded Load Path

The L2 forwarded load path calls `completion_from_forwarded_load` directly and currently does not allocate a DMEM request or txid. It relies on `lsu_finish_forwarded_load` to reclaim the LQ entry after completion. L3 must preserve that behavior: forwarded loads consume no external outstanding slot.

## Completion Holding And Backpressure

There is one durable internal held load-response completion slot: `state.completion.load_response`. `completion_service_cycle` only reads a new `pipe.dmem_resp` response when this slot is not valid. `boom_core_top` only reads `dmem_resp_in` into `pipe.dmem_resp` when `pipe.dmem_resp` is not full. This is sufficient for a single response channel, provided stale responses are allowed to be discarded and valid responses remain held until completion service accepts them.

## L2 Semantics To Preserve

The existing store-ordering decision in `plan_load_issue` implements the L2 contract:

1. Known non-alias older stores permit memory load.
2. The youngest overlapping full-cover store forwards.
3. Partial overlap blocks.
4. Multi-store merge is absent.
5. Memory plus forward merge is absent.
6. Memory order speculation, violation detection, and replay are absent.
7. Store external side effects remain at precise commit.

L3 must only remove the global load-pending issue restriction and must not weaken these decisions.
