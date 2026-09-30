# Gate 6.0 L2 Forwarding Contract

## Policy

`YOUNGEST_OLDER_OVERLAPPING_SINGLE_STORE_FULL_COVER_ONLY`

For a load ready to issue:

1. Walk valid architecturally older ROB entries using canonical head-relative ROB order.
2. Ignore all architecturally younger stores.
3. If any older store has not resolved address/data into a live exact SQ owner, return `BLOCK`.
4. For each resolved older store, compare its valid absolute byte footprint with every requested load byte.
5. If no older store overlaps any requested load byte, return `MEMORY` and use the unchanged single-outstanding external load path.
6. If stores overlap, select the youngest older overlapping store.
7. Return `FORWARD` only if that selected store alone covers every requested load byte.
8. If the selected store covers only part of the load, return `BLOCK`, even if an older store or memory could provide the remaining bytes.

## Consequences

- A known non-alias older store does not block normal memory issue.
- A younger partial overlap overrides an older full-cover candidate and causes `BLOCK`.
- Two or more stores are never merged.
- Store bytes and memory bytes are never merged.
- A wide store may satisfy a narrower contained load.
- A narrow store cannot satisfy a wider load.
- Selected bytes are assembled by absolute address and use canonical load sign/zero extension for LB/LBU/LH/LHU/LW/LWU/LD.
- A forwarded load emits no DMEM request and creates no external pending transaction.
- A forwarded result enters the canonical retained completion path. It must survive completion backpressure and must still pass ROB owner and LQ owner validation before architectural mutation.
- Branch squash, exception/global flush, and reset invalidate a retained forwarded completion exactly as they invalidate other canonical completions.
- A store that is still in ROB/SQ because its commit request is backpressured remains eligible. The canonical implementation has no post-acceptance committed-but-undrained SQ state.

## Explicit Non-Features

- Partial forwarding: false.
- Multi-store forwarding merge: false.
- Memory-plus-forward merge: false.
- Load speculation: false.
- Memory-order violation detection: false.
- Load replay: false.
- Multiple outstanding external loads: false.
- Store address/data decoupling: false.
- DCache, MMU, L3, and L4 functionality: absent.

## Decision Oracle

The independent test oracle consumes an ordered older-store set and returns:

- `BLOCK` if any older store is unresolved or if the youngest older overlap is partial.
- `MEMORY` if every older store is resolved and none overlaps.
- `FORWARD(selected_store, bytes, extended_value)` if the youngest older overlap alone fully covers the load.

The test oracle must not call or duplicate product decision helpers.
