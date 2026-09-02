# Predictor Latency Contract

`RECOMMENDED_PREDICTOR_LATENCY_MODEL=FIXED_1_CYCLE_BLOCKING_PACKET`

| Latency | Frontend consequence | Assessment |
|---|---|---|
| 0 | combinational predecode, table read, selection, target add, mask, and next-PC path in packet cycle | simple protocol but high HLS timing risk |
| 1 | predecode issues one exact-CFI request; completed packet is retained; matching response finalizes mask/next PC then atomic enqueue | recommended explicit state machine and measurable table read |
| 2+ | longer packet hold, more request identity/state, larger fetch bubble with one outstanding IMEM transaction | defer until a pipelined frontend/block predictor exists |

The foundation predictor does not have to run in parallel with IMEM because an accurate BIM index is unavailable before CFI predecode. IMEM-parallel lookup becomes useful with a BTB/fetch-block design, which is outside foundation.

If response is later than packet construction, the packet waits in a dedicated pending-prediction state. It must not enter the Fetch Buffer or allocate FTQ until prediction completes. The pending token contains exact CFI PC/lane, epoch, static target, packet mask, and packet payload ownership. Only one pending prediction is allowed initially.

Packets with no valid predecoded CFI, including ordinary non-CFI and fault-only packets, issue no predictor request and bypass the predictor without a one-cycle delay. They retain the parser's sequential next PC and mask.

Late prediction redirects after enqueue are forbidden in foundation: they create avoidable same-packet/FTQ recovery races. Timeout is not silently treated as a response; an explicit unavailable response may select sequential fallback. The fixed one-cycle contract is preferable for Vitis HLS because it separates memory lookup from frontend combinational parsing without requiring `CORE_CYCLE` pipelining or a variable-latency queue.
