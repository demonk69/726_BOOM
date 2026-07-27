# Branch Recovery HLS Mapping

Gate 3.4 confirmed the Gate 3.3 branch recovery structures in generated HLS RTL. Gate 3.5 tested structural alternatives but accepted none, so this remains the accepted mapping:

| Structure | HLS Source | Generated RTL Structure | Status |
|---|---|---|---|
| Rename map snapshots | `rename.int_map_table.br_snapshots[32][8]` | `RAM_AUTO_1R1W`, 256x8 reg-array RAM | Implemented for supported subset |
| Branch allocation lists | `branch_state.br_alloc_lists[8][52]` | `RAM_AUTO_1R1W`, 416x1 reg-array RAM | Implemented for supported subset |
| Branch mask kill | ROB/IQ/Execute/LDQ/STQ branch masks | Entry-wise compare and helper-level kill/compact logic | Implemented for supported subset |
| Busy recovery | `rebuild_busy_after_recovery` | 52-bit clear plus 32-entry ROB scan inside `recover_mispredict` | Functional substitute |

The original Chisel checkout remains absent. BOOM source mapping is still based on generated SmallBoomConfig FIRRTL/Verilog; see `docs/branch_recovery_source_mapping.md`.

Gate 3.5 notes: `B1_PACKED_ALLOC_BITMAP` changed the allocation-list storage shape to 8x52 `RAM_AUTO_1R1W` during the experiment, but it increased full-core LUT and was rejected. The accepted mapping remains 416x1 `RAM_AUTO_1R1W`.
