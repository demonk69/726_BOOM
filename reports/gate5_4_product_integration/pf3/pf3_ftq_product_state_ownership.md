# PF3 FTQ Product State Ownership

- `BoomCoreState::ftq` is the only canonical product FTQ state and instantiates
  `FtqFoundation<FTQ_DEPTH>`.
- Frontend holds only pending admission context and the returned allocation
  reference.
- Fetch Buffer instructions and `MicroOp`/ROB entries carry the 40-bit
  reference plus the separate existing `is_rvc` attribute.
- ROB entries do not copy `FtqEntry` payload.
- Commit and existing backend redirect logic emit generation-qualified terminal
  events. The canonical FTQ alone owns entry payload, live masks, pointers,
  count, and allocation generation.
