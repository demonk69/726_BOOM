# Gate 6.0 L3 Outstanding Contract

Architecture: `PER_LQ_ENTRY_OUTSTANDING_OWNERSHIP`

Feasible: `true`

Maximum outstanding loads:

| LQ_DEPTH | MAX_OUTSTANDING_LOADS |
| --- | ---: |
| 4 | 4 |
| 8 | 8 |
| 16 | 16 |

Contract points:

| Requirement | Status | Evidence |
| --- | --- | --- |
| One external transaction per LQ entry | PASS | `LoadQueueEntry::response_pending` blocks same-entry reissue |
| Active owner identity includes txid, LQ index, LQ generation, ROB index, ROB allocation ID | PASS | `completion_from_load_response` scans active LQ entries, then validates ROB/LQ tuple |
| Out-of-order load responses | PASS | Native and CSim L3 tests complete B before A and C,A,B |
| Duplicate response rejected | PASS | L3 stale/duplicate cases |
| Stale after LQ/ROB reuse rejected | PASS | L3 reuse cases |
| Forwarded load consumes no external slot | PASS | L3 forwarded case and L2 preservation |
| Request backpressure creates no phantom owner | PASS_NATIVE | Native shim supports forced-full stream; Vitis CSim cannot force full with default stream |
| Completion holding does not overwrite response | PASS | L3 holding case |
| Single request channel preserved | PASS | One external load request per `lsu_module` call |
| Single response channel preserved | PASS | One held `state.completion.load_response`; one response read per cycle |
| Speculation/replay not introduced | PASS | Store-ordering decision unchanged |

`CURRENT_MAX_OUTSTANDING_LOADS=LQ_DEPTH`

`CURRENT_MEMORY_ORDER_SPECULATION=false`

`CURRENT_MEMORY_ORDER_VIOLATION_DETECTION=false`

`CURRENT_LOAD_REPLAY=false`
