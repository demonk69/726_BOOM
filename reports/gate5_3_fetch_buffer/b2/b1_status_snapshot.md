# B1 Status Snapshot

- `GATE5_3_B1_FETCH_BUFFER_VERIFIED=true`
- `GATE5_3_B1_STANDALONE_ONLY=true`
- `GATE5_3_B1_CANONICAL_DEPTH=8`
- `GATE5_3_B1_CANONICAL_STORAGE=AUTO`
- `GATE5_3_B1_RESET_POLICY=CONTROL_ONLY`
- Enqueue: four lanes, compacted ascending lane order, atomic masked acceptance.
- Dequeue: one entry per step.
- Flush: higher priority than enqueue and dequeue.
- Depth-8 AUTO PPA: 1179 LUT, 846 FF, 0 BRAM, 0 DSP, 4.574 ns.

B2 preserves this core and uses only the legal `valid_mask=0x1` product subset.
