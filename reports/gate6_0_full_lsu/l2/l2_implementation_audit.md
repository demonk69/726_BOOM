# Gate 6.0 L2 Implementation Audit

## Pre-Implementation Baseline

- Baseline commit: `eb753342642ce6bcff2c6007283d6206c9ae3a14`.
- Branch: `gate3.8-rtl-verification`; baseline ahead/behind: `0/0`; index empty.
- T0R canonical result: 201071 LUT, 45693 FF, 16 BRAM, 3 DSP, 6.071 ns.
- T0R critical path: `build_fetch_packet`.
- Baseline product aggregate: `492cc6257137595a910273e20f77c08693146ac927b342045c1a6e71ec6284a9`.
- Baseline merged source: `106a5ff727ddd3798976e80222c01f69d9359990a36afadd7566dc2241e3c4c1`.
- Final product aggregate: `7fa60c73d7c2a584ecb859e8d4f6ec5270ad5d8d6da9751030af6becdc68d808`.
- Final merged source: `25684f6a7ad91b40ea0f36389b1ea4d1abbbf16e0663174aec2daa02649a33fe`.

## Canonical State And Lifecycle

- `StoreQueueEntry` contains valid, address/data-valid, ROB index/allocation ID, 16-bit generation, mask, size, and branch mask. The 64-bit address and data payloads are parallel `LsuState` arrays with the same SQ index and lifecycle (`include/boom_state.hpp`).
- `LoadQueueEntry` contains valid, ROB index/allocation ID, 16-bit generation, address, size, signedness, transaction, and response ownership (`include/boom_state.hpp:247-256`).
- `LsuState` has independently parameterized LQ/SQ arrays and one external pending-load tuple (`include/boom_state.hpp:258-287`).
- MEM execute completion atomically allocates a store queue entry with address and data both valid (`src/lsu.cpp:82-139`). There is no product transition that allocates a valid SQ entry with unresolved address or data.
- An unresolved older store is nevertheless representable in the ROB before its MEM execute completion. Therefore forwarding eligibility cannot be decided by scanning valid SQ entries alone.
- Loads are allocated after address generation, then issued by a head-relative ROB scan (`src/lsu.cpp:195-243`).
- The replaced L1 policy blocked a load on any older valid ROB store, independent of address. The final L2 implementation uses the three-way issue plan described below.
- Store external side effects occur only at precise ROB-head commit. A store remains in ROB/SQ while the external request is backpressured and is reclaimed immediately when that request is accepted (`src/commit.cpp:176-255`). There is no separate committed-but-not-drained state after request acceptance.
- External load responses validate transaction ID, ROB index/allocation ID, LQ index/generation, and LQ response state before entering the retained completion path (`src/completion.cpp:73-121`).
- Completion is retained under writeback/backpressure and serviced in ROB age order (`src/completion.cpp:625-745`).
- Correct branch resolution clears masks; mispredict, exception flush, and reset remove killed queue owners and retained completions (`src/branch.cpp:69-95,138-185`; `src/commit.cpp:40-83`; `src/reset.cpp:226-270`).

## Address And Data Semantics

- Sizes `0/1/2/3` mean 1/2/4/8 bytes and cover LB/LBU/LH/LHU/LW/LWU/LD (`src/decode.cpp:165-184`).
- Store address is the exact first-byte address. Store mask bit `i` and store data byte `i` describe byte `address+i` (`src/execute.cpp:169-176`; `tb/differential/lsu_minimal_tests.cpp:53-64`).
- A memory load response is an aligned 64-bit word; existing completion logic shifts by `load_address[2:0]` before sign/zero extension (`src/completion.cpp:21-35`).
- Forwarding must therefore compare absolute byte addresses, assemble selected store bytes into low-order load bytes, and then apply the same extension semantics as a memory response.

## Identity And Ordering

- ROB lifetime identity is `(rob_idx, rob_allocation_id)`.
- Queue ownership adds `(queue_index, queue_generation)`; generations are 16 bits and increment on slot allocation.
- Canonical age is head-relative ROB order, not raw numeric index comparison. The existing LSU scan from `rob.head` to the load naturally handles wraparound.
- A resolved store may participate only if its ROB entry and referenced SQ entry agree on index, generation, ROB index, and allocation ID, and the SQ address/data remain valid.
- Stores after the candidate load in head-relative ROB order are architecturally younger and are ignored.

## Minimal Product Change

- Replace the L1 boolean older-store barrier with a three-way `BLOCK`, `MEMORY`, or `FORWARD` decision in `src/lsu.cpp`.
- Scan older ROB entries to preserve unresolved-store blocking and canonical wraparound semantics. Track the last overlapping resolved store encountered, which is the youngest older overlap.
- Separate metadata selection from data extraction: select one SQ index first, then read only that entry's data.
- Add an explicit internal-forward marker to the existing retained LSU completion event. A forwarded load does not emit a DMEM request and does not populate the external pending-load tuple.
- Reclaim the exact LQ owner only after the retained forwarded completion is accepted. This preserves completion backpressure, ROB/PRF semantics, branch masks, and stable identity.
- No LQ/SQ depth or width reduction, replay queue, second external load, store protocol change, or pipeline stage was introduced. The L2R repair moved SQ address/data payloads into parallel arrays and applies targeted cyclic factor-2 partitioning only for `SQ_DEPTH >= 16`; `directive_audit.md` records the accepted directive scope.

## Frozen Architecture

- `MAX_OUTSTANDING_LOADS=1` for external memory.
- `STORE_ADDRESS_DATA_DECOUPLING=NONE`.
- `MEMORY_ORDER_SPECULATION=false`.
- `MEMORY_ORDER_VIOLATION_DETECTION=false`.
- `LOAD_REPLAY=false`.
- LQ/SQ defaults remain 8/8 and supported configurations remain 4/4, 8/8, 16/16, 4/16, and 16/4.
