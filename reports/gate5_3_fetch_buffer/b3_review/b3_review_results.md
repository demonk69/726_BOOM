# Gate 5.3 B3 Fetch Packet Construction Architecture Review

## Required Outputs

```text
IMEM_RESPONSE_BITS=32
IMEM_RESPONSE_BYTES=4
PARCELS_PER_RESPONSE=2

MAX_COMPLETE_INSTRUCTIONS_PER_RESPONSE_ALL_RVC=2
MAX_COMPLETE_INSTRUCTIONS_PER_RESPONSE_ALL_32=1
MAX_COMPLETE_INSTRUCTIONS_PER_RESPONSE_MIXED=2

CURRENT_PRODUCT_ENQUEUE_WIDTH=1
CURRENT_DECODE_WIDTH=1
CURRENT_DISPATCH_WIDTH=1

MULTI_RESPONSE_PACKET_AGGREGATION_REQUIRED_FOR_WIDTH4=true

RECOMMENDED_PACKET_WIDTH=2
RECOMMENDATION=OPTION_B_IMPLEMENT_2_LANE_PACKET
READY_FOR_PACKET_IMPLEMENTATION=true
```

## Decision

The canonical IMEM response is one little-endian 32-bit word, not the configured future eight-byte ICache fetch width. It contains two 16-bit parcels and can naturally form at most two complete instructions. B1's four-lane atomic enqueue is storage-side capacity; it does not establish a reachable four-wide producer.

Choose `OPTION_B_IMPLEMENT_2_LANE_PACKET`: unpack only the current matched response plus existing carry into a contiguous ordered packet of zero, one, or two complete entries. Keep one locally tracked outstanding-request ownership record, depth-8 storage, one-wide dequeue, Decode/Dispatch/Commit width one, and all current identity/epoch/stale-drain rules. Redirected old external transactions may coexist physically and must remain stale.

Width 2 can:

- publish C+C together;
- complete a carried 32-bit instruction as lane 0 and use response B's upper C as lane 1;
- retain a new upper 32-bit start as carry after lane 0 completion;
- raise the measured alternating C/32 Frontend supply from 0.8 toward the 4/3 instruction/response bound;
- combine carry completion with an upper compressed instruction when both complete in response B.

Retaining response B's upper parcel as a new carry can raise upper-start all-32 supply from measured 0.5 toward 1 instruction/response, but that is a parser scheduling improvement, not an intrinsic width-2 benefit; an optimized width-1 producer could do the same. Width 2 is justified by cases with two complete entries, especially C+C and carry+C.

It cannot raise long-run native-call throughput above one accepted instruction per call because Decode, Dispatch, and Commit remain one-wide. At 25% or 50% Decode stalls, accepted B2 data already shows approximately 0.75 and 0.5 instructions per native architectural call; wider packet production only changes burst fill before the buffer reaches capacity. These are C++ call-level bounds, not `ap_clk`, HLS initiation-interval, or physical-clock IPC claims. B3I requires fresh synthesis and generated-RTL measurement.

Width 4 is rejected for the current interface. Filling four lanes requires cross-response accumulation, which is a second packet buffer in front of the Fetch Buffer, increases fault/flush/latency state, and provides no all-32 or backend throughput gain. Revisit four-wide construction only with a real wider ICache/fetch response.

## Required B3I Contract

- Packet lanes are contiguous and strictly ordered; partial carry is never a lane.
- First ordered fault terminates the packet; no younger lane is admitted.
- Packet hold is stable until atomic enqueue or flush.
- Runtime reset and every accepted redirect kill the entire unpublished packet and existing carry/response state.
- Same-cycle redirect wins over response; exact `{fetch_id, epoch, address}` matching and stale drain are unchanged.
- No FTQ, predictor, BTB/BIM/TAGE/RAS, ICache, Decode/backend widening, Full LSU, FPU, DATAFLOW, false dependence, complete array partition, or core-cycle pipeline is permitted.

## Evidence Index

- `imem_packet_capacity.md`: exact interface, ordering, retention, and maximum instructions.
- `packet_width_tradeoff.csv`: width 1/2/4 feasibility and risk.
- `packet_fault_semantics.md`: precise exception and cross-word termination.
- `packet_throughput_model.csv`: measured and theoretical throughput bounds with assumptions.
- `architecture_boundary.md`: response unpacking versus multi-response aggregation and redirect scope.
- `ppa_risk_analysis.md`: decompressor, mux/state, and timing risk.

This phase performed architecture review only. No packet constructor or product-source change was made.
