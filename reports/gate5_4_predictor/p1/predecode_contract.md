# Standalone CFI Predecode Contract

`predecode_cfi(pc, instruction, is_rvc)` consumes one complete canonical 32-bit instruction. RV64C decompression has already happened; `is_rvc` supplies only original length metadata. The function is combinational, deterministic, stateless, and contains no mutable static data, queue, table, history, allocation identity, or product-state dependency.

The result identifies only legal encodings of BEQ/BNE/BLT/BGE/BLTU/BGEU, JAL, and JALR (`funct3=0`). Direct branch/JAL targets use modulo-2^64 `pc + sign_extended_immediate`. JALR target is unavailable because it needs `rs1`; `static_target_valid` is false. Non-CFI and reserved branch/JALR encodings return `CFI_NONE` and cannot bypass architectural Decode legality.

The scalar API has no input-valid or exception input because it is a standalone complete-instruction helper, so `result.valid=true`. A packet caller controls slot validity with `valid_mask`; fault/illegal validity remains owned by product packet construction/Decode. P1 does not connect this interface to that product path.

`predecode_cfi_packet` accepts masks `00`, `01`, and `11`, computes both lanes combinationally, and selects lane 0 before lane 1. `younger_lane_mask` identifies valid same-packet lanes younger than the selected CFI. `mask_younger_packet_lanes` proves the future predicted-taken mask transformation but performs no prediction.
