# B2 Frontend Buffer Contract Snapshot

The normative contract is `docs/fetch_buffer_frontend_contract.md`.

- Frontend producer width: one complete canonical instruction.
- B1 use: lane 0 only, `valid_mask=1`; packet capability remains intact.
- Payload: PC, instruction, fetch ID, RVC indication, exception, exception cause.
- Excluded payloads: carry halfword, partial 32-bit instruction, stale/killed response, invalid decompressor result without its ordered illegal-instruction fault representation.
- Decode reads only the FIFO head; no empty-FIFO bypass exists.
- Runtime reset and all accepted redirects/flushes suppress both old dequeue and old enqueue and clear FIFO control state.
