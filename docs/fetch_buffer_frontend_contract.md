# Frontend to Fetch Buffer Contract

## Product Boundary

Gate 5.3 B2 inserts the accepted depth-8 B1 Fetch Buffer between the canonical Frontend and one-wide Decode. The product producer width is exactly one instruction. It drives only B1 lane 0 with `valid_mask=0x1`; the standalone four-lane atomic packet interface remains available but B2 does not construct multi-instruction packets.

`FetchInstruction` is the product `FetchBufferEntry` and contains:

- `pc`: architectural PC of the complete instruction.
- `instruction`: canonical 32-bit instruction presented to Decode.
- `fetch_id`: identity of the validated IMEM response completing the instruction.
- `is_rvc`: whether the source instruction was compressed.
- `exception`: ordered instruction-fetch/decompression fault indication.
- `exception_cause`: architectural cause associated with `exception`.

Only complete instructions are entries. A carry halfword, a partial cross-word 32-bit instruction, a stale or killed response, and a response rejected by fetch ID/epoch/address validation cannot enter the buffer. An illegal compressed encoding is represented only by its complete ordered canonical fault entry; no invalid decompressor payload is enqueued as an ordinary instruction.

## Ownership and Handshake

Frontend owns one producer holding register. Once valid, all entry fields remain stable until B1 reports lane-0 enqueue acceptance or a higher-priority flush kills the entry. Full capacity therefore backpressures Frontend without overwrite or duplicate enqueue.

The Fetch Buffer owns accepted entries. Decode sees only the current FIFO head. Decode availability is the buffer `dequeue_valid`; Decode capacity is `dequeue_ready`. An entry is removed only on `dequeue_valid && dequeue_ready`. B2 deliberately has no empty-buffer producer-to-Decode bypass.

Frontend may continue accepting validated responses and issuing the next logical request while Decode is stalled until the FIFO plus producer hold reaches capacity. Partial cross-word carry remains private Frontend state.

## Flush Priority

The control priority is runtime reset, owned architectural redirect, branch redirect, generic flush, then normal push/pop. Any accepted flush clears producer hold, FIFO head/tail/count, retained response, carry, and outstanding request identity for the old epoch. On a flush cycle old FIFO entries are not visible, no old entry dequeues, and no old producer entry enqueues. Redirect fetch restarts under the new epoch.

Instruction-fetch faults are ordinary FIFO entries and preserve PC/cause ordering. A reset or redirect before Decode consumption removes the fault and prevents a late exception.

## Frozen Scope

Decode and Dispatch remain width one. No FTQ, predictor, RAS, BTB/BIM/TAGE, ICache, packet constructor, Full LSU, FPU, DATAFLOW, false dependence, complete array partition, or core-cycle pipeline is introduced. `src/boom_all.cpp` is excluded.
