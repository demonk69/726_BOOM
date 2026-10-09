# Gate 6.0 L3R2 RTL Generation

## CONFIG_4_4 Result

Status: PASS

Canonical source worktree: `/home/lab_726/opencode_tmp/g6_l3r2_79_clean`

Canonical commit: `79cb9cf8a965fa8f25e036be6f42df8178a5f8b6`

HLS top: `boom_core_top`

HLS source: `src/boom_core_merged.cpp`

Config: `LQ_DEPTH=4`, `SQ_DEPTH=4`

Generation script: `scripts/gate6_0/l2_full_core_csynth.tcl`

Command evidence: `logs/config_4_4_csynth.log`

Generated project: `/home/lab_726/opencode_tmp/g6_l3r2/rtl_generation/4_4/hls_project`

Generated RTL: `/home/lab_726/opencode_tmp/g6_l3r2/rtl_generation/4_4/hls_project/solution/syn/verilog`

Generated RTL files: 93 Verilog files and 32 `.dat` files

Generated RTL aggregate hash: `2c1d683260694cb949658c15ce59564cf721e371d25a176a658489c0aadeb906`

## Tool Evidence

Vitis HLS: `2021.2`, SW build `3367213`

Vivado simulator: `xvlog`/`xsim` `2021.2`

Top timing estimate: target `10.00 ns`, estimated `6.071 ns`

Top resource estimate: `BRAM_18K=16`, `DSP=3`, `FF=47985`, `LUT=211204`, `URAM=0`

Top latency summary: unbounded full-core `CORE_CYCLE`; reset submodule estimated `1..260` cycles

## Interface Evidence

Generated top ports include AXIS-style streams:

`dmem_req_out_TDATA[319:0]`, `dmem_req_out_TVALID`, `dmem_req_out_TREADY`

`dmem_resp_in_TDATA[383:0]`, `dmem_resp_in_TVALID`, `dmem_resp_in_TREADY`

Observed DmemRequest TDATA packing used by the RTL scoreboard:

`[31:0] transaction_id`, `[39:32] rob_idx`, `[47:40] command`, `[55:48] size`, `[63:56] mask`, `[191:128] address`

Observed DmemResponse TDATA packing used by the RTL scoreboard:

`[31:0] transaction_id`, `[127:64] data`, `[191:128] read_data`

## L3 Ownership Evidence

The generated CONFIG_4_4 RTL preserves L3 load ownership state and functions:

`state_lsu_ldq_valid_0..3`

`state_lsu_ldq_response_pending_0..3`

`state_lsu_ldq_transaction_id_0..3`

`state_lsu_ldq_rob_idx_0..3`

`state_lsu_ldq_rob_allocation_id_0..3`

`state_lsu_ldq_generation_0..3`

`state_lsu_outstanding_load_count_V`

`boom_core_top_try_issue_load.v`

`boom_core_top_lsu_finish_load_response.v`

`boom_core_top_completion_service_cycle.v`

## Scope

CONFIG_4_4 full-core RTL generation and simulation passed.

Extended full-core RTL configs `8_8`, `16_16`, `4_16`, and `16_4` were not generated or simulated in this L3R2 run, so they are not marked RTL PASS here.
