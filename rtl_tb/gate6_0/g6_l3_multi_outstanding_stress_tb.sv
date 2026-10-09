`timescale 1ns/1ps

`ifndef G6_L3_LQ_DEPTH
`define G6_L3_LQ_DEPTH 4
`endif

`ifndef G6_L3_SQ_DEPTH
`define G6_L3_SQ_DEPTH 4
`endif

`ifdef G6_L3_HAS_LQ8
`define LQ_VALID_CASE(N) N: lq_valid = dut.state_lsu_ldq_valid_U.ram[N];
`define LQ_PENDING_CASE(N) N: lq_pending = dut.state_lsu_ldq_response_pending_U.ram[N];
`define LQ_TXID_CASE(N) N: lq_txid = dut.state_lsu_ldq_transaction_id_U.ram[N];
`define LQ_ROB_CASE(N) N: lq_rob_idx = dut.state_lsu_ldq_rob_idx_U.ram[N];
`define LQ_ALLOC_CASE(N) N: lq_alloc = dut.state_lsu_ldq_rob_allocation_id_U.ram[N];
`define LQ_GEN_CASE(N) N: lq_generation = dut.state_lsu_ldq_generation_U.ram[N];
`define SEED_LQ_CASE(N) \
    N: begin \
        dut.state_lsu_ldq_valid_U.ram[N] = 1'b1; \
        dut.state_lsu_ldq_response_pending_U.ram[N] = 1'b0; \
        dut.state_lsu_ldq_rob_idx_U.ram[N] = rob_idx; \
        dut.state_lsu_ldq_branch_mask_U.ram[N] = branch_mask; \
        dut.state_lsu_ldq_transaction_id_U.ram[N] = 32'd0; \
        dut.state_lsu_ldq_rob_allocation_id_U.ram[N] = allocation; \
        dut.state_lsu_ldq_generation_U.ram[N] = generation; \
    end
`else
`define LQ_VALID_CASE(N) N: lq_valid = dut.state_lsu_ldq_valid_``N;
`define LQ_PENDING_CASE(N) N: lq_pending = dut.state_lsu_ldq_response_pending_``N;
`define LQ_TXID_CASE(N) N: lq_txid = dut.state_lsu_ldq_transaction_id_``N;
`define LQ_ROB_CASE(N) N: lq_rob_idx = dut.state_lsu_ldq_rob_idx_``N;
`define LQ_ALLOC_CASE(N) N: lq_alloc = dut.state_lsu_ldq_rob_allocation_id_``N;
`define LQ_GEN_CASE(N) N: lq_generation = dut.state_lsu_ldq_generation_``N;
`define SEED_LQ_CASE(N) \
    N: begin \
        dut.state_lsu_ldq_valid_``N = 1'b1; \
        dut.state_lsu_ldq_response_pending_``N = 1'b0; \
        dut.state_lsu_ldq_rob_idx_``N = rob_idx; \
        dut.state_lsu_ldq_branch_mask_``N = branch_mask; \
        dut.state_lsu_ldq_transaction_id_``N = 32'd0; \
        dut.state_lsu_ldq_rob_allocation_id_``N = allocation; \
        dut.state_lsu_ldq_generation_``N = generation; \
    end
`endif

`ifdef G6_L3_CYCLE_FU_6331
`define G6_L3_INTERNAL_DMEM_TDATA dut.grp_boom_core_cycle_io_fu_6331_dmem_req_out_TDATA
`define G6_L3_INTERNAL_DMEM_TVALID dut.grp_boom_core_cycle_io_fu_6331_dmem_req_out_TVALID
`define G6_L3_INTERNAL_DMEM_TREADY dut.grp_boom_core_cycle_io_fu_6331_dmem_req_out_TREADY
`elsif G6_L3_CYCLE_FU_6373
`define G6_L3_INTERNAL_DMEM_TDATA dut.grp_boom_core_cycle_io_fu_6373_dmem_req_out_TDATA
`define G6_L3_INTERNAL_DMEM_TVALID dut.grp_boom_core_cycle_io_fu_6373_dmem_req_out_TVALID
`define G6_L3_INTERNAL_DMEM_TREADY dut.grp_boom_core_cycle_io_fu_6373_dmem_req_out_TREADY
`elsif G6_L3_CYCLE_FU_6317
`define G6_L3_INTERNAL_DMEM_TDATA dut.grp_boom_core_cycle_io_fu_6317_dmem_req_out_TDATA
`define G6_L3_INTERNAL_DMEM_TVALID dut.grp_boom_core_cycle_io_fu_6317_dmem_req_out_TVALID
`define G6_L3_INTERNAL_DMEM_TREADY dut.grp_boom_core_cycle_io_fu_6317_dmem_req_out_TREADY
`elsif G6_L3_CYCLE_FU_6257
`define G6_L3_INTERNAL_DMEM_TDATA dut.grp_boom_core_cycle_io_fu_6257_dmem_req_out_TDATA
`define G6_L3_INTERNAL_DMEM_TVALID dut.grp_boom_core_cycle_io_fu_6257_dmem_req_out_TVALID
`define G6_L3_INTERNAL_DMEM_TREADY dut.grp_boom_core_cycle_io_fu_6257_dmem_req_out_TREADY
`else
`define G6_L3_INTERNAL_DMEM_TDATA dut.grp_boom_core_cycle_io_fu_6197_dmem_req_out_TDATA
`define G6_L3_INTERNAL_DMEM_TVALID dut.grp_boom_core_cycle_io_fu_6197_dmem_req_out_TVALID
`define G6_L3_INTERNAL_DMEM_TREADY dut.grp_boom_core_cycle_io_fu_6197_dmem_req_out_TREADY
`endif

`ifdef G6_L3_HAS_SQ16
`define G6_L3_STQ_VALID0 dut.state_lsu_stq_valid_U.ram[0]
`define G6_L3_STQ_ADDR_VALID0 dut.state_lsu_stq_address_valid_U.ram[0]
`define G6_L3_STQ_DATA_VALID0 dut.state_lsu_stq_data_valid_U.ram[0]
`define G6_L3_STQ_ROB0 dut.state_lsu_stq_rob_idx_U.ram[0]
`define G6_L3_STQ_MASK0 dut.state_lsu_stq_mask_U.ram[0]
`define G6_L3_STQ_ALLOC0 dut.state_lsu_stq_rob_allocation_id_U.ram[0]
`define G6_L3_STQ_GEN0 dut.state_lsu_stq_generation_U.ram[0]
`define G6_L3_STQ_ADDR0 dut.boom_core_top_stream_stream_stream_DmemRequest_0_stream_DmemResponse_0_stream_CommitEntry_0_bool_volatile_bool_volatile_bool_volatile_bool_volatile_unsigned_long_volatile_unsigned_long_volatile_state_U.ram[0]
`define G6_L3_STQ_DATA0 dut.boom_core_top_stream_stream_stream_DmemRequest_0_stream_DmemResponse_0_stream_CommitEntry_0_bool_volatile_bool_volatile_bool_volatile_bool_volatile_unsigned_long_volatile_unsigned_long_volatile_state_2_U.ram[0]
`elsif G6_L3_HAS_SQ8
`define G6_L3_STQ_VALID0 dut.state_lsu_stq_valid_U.ram[0]
`define G6_L3_STQ_ADDR_VALID0 dut.state_lsu_stq_address_valid_U.ram[0]
`define G6_L3_STQ_DATA_VALID0 dut.state_lsu_stq_data_valid_U.ram[0]
`define G6_L3_STQ_ROB0 dut.state_lsu_stq_rob_idx_U.ram[0]
`define G6_L3_STQ_MASK0 dut.state_lsu_stq_mask_U.ram[0]
`define G6_L3_STQ_ALLOC0 dut.state_lsu_stq_rob_allocation_id_U.ram[0]
`define G6_L3_STQ_GEN0 dut.state_lsu_stq_generation_U.ram[0]
`define G6_L3_STQ_ADDR0 dut.state_lsu_stq_address_U.ram[0]
`define G6_L3_STQ_DATA0 dut.state_lsu_stq_data_U.ram[0]
`else
`define G6_L3_STQ_VALID0 dut.state_lsu_stq_valid_0
`define G6_L3_STQ_ADDR_VALID0 dut.state_lsu_stq_address_valid_0
`define G6_L3_STQ_DATA_VALID0 dut.state_lsu_stq_data_valid_0
`define G6_L3_STQ_ROB0 dut.state_lsu_stq_rob_idx_0
`define G6_L3_STQ_MASK0 dut.state_lsu_stq_mask_0
`define G6_L3_STQ_ALLOC0 dut.state_lsu_stq_rob_allocation_id_0
`define G6_L3_STQ_GEN0 dut.state_lsu_stq_generation_0
`define G6_L3_STQ_ADDR0 dut.state_lsu_stq_address_0
`define G6_L3_STQ_DATA0 dut.state_lsu_stq_data_0
`endif

module g6_l3_multi_outstanding_stress_tb;
    localparam integer LQ_DEPTH = `G6_L3_LQ_DEPTH;
    localparam integer SQ_DEPTH = `G6_L3_SQ_DEPTH;
    localparam integer MAX_TRACKED = 128;
    localparam [31:0] TX_BASE = 32'h0000_1000;
    localparam [63:0] ADDR_BASE = 64'h0000_0000_8000_0000;
    localparam [63:0] DATA_BASE = 64'h7000_0000_0000_0000;

    reg clk = 0;
    reg rst_n = 0;
    wire [191:0] imem_req_tdata;
    wire imem_req_tvalid;
    wire [319:0] dmem_req_tdata;
    wire dmem_req_tvalid;
    reg dmem_req_tready = 1'b1;
    wire [383:0] dmem_resp_tdata_unused;
    reg [383:0] dmem_resp_tdata = 384'd0;
    reg dmem_resp_tvalid = 1'b0;
    wire dmem_resp_tready;
    wire [767:0] trace_tdata;
    wire trace_tvalid;
    wire ap_local_block, ap_local_deadlock;
    wire io_success, io_halted, io_trap, io_cycle_valid;
    wire [63:0] io_cycle, io_instret;

    integer cycles = 0;
    integer request_count = 0;
    integer response_count = 0;
    integer case_count = 0;
    integer case_failures = 0;
    integer max_observed_outstanding = 0;
    integer response_backpressure_blocked_cycles = 0;
    integer stale_response_reject_count = 0;
    reg [8*48-1:0] current_case = "init";

    reg [31:0] req_txid [0:MAX_TRACKED-1];
    reg [7:0] req_rob [0:MAX_TRACKED-1];
    reg [7:0] req_command [0:MAX_TRACKED-1];
    reg [7:0] req_size [0:MAX_TRACKED-1];
    reg [7:0] req_mask [0:MAX_TRACKED-1];
    reg [63:0] req_address [0:MAX_TRACKED-1];
    reg [31:0] slot_txid [0:15];
    reg [63:0] slot_data [0:15];
    reg [31:0] slot_alloc [0:15];
    reg [15:0] slot_generation [0:15];

    assign dmem_resp_tdata_unused = dmem_resp_tdata;

    boom_core_top dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk), .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata), .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(1'b0), .imem_resp_in_TDATA(256'd0),
        .imem_resp_in_TVALID(1'b0), .imem_resp_in_TREADY(),
        .dmem_req_out_TDATA(dmem_req_tdata), .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(dmem_req_tready), .dmem_resp_in_TDATA(dmem_resp_tdata_unused),
        .dmem_resp_in_TVALID(dmem_resp_tvalid), .dmem_resp_in_TREADY(dmem_resp_tready),
        .commit_trace_out_TDATA(trace_tdata), .commit_trace_out_TVALID(trace_tvalid),
        .commit_trace_out_TREADY(1'b1), .io_success(io_success),
        .io_halted(io_halted), .io_trap(io_trap),
        .io_cycle_valid(io_cycle_valid), .io_cycle(io_cycle), .io_instret(io_instret));

    always #5 clk = ~clk;

    always @(posedge clk) begin
        cycles <= cycles + 1;
        if (cycles > 1000000) $fatal(1, "G6_L3_R2_RTL_FAIL timeout");
        if (rst_n) begin
            if (dut.state_lsu_outstanding_load_count_V > max_observed_outstanding)
                max_observed_outstanding <= dut.state_lsu_outstanding_load_count_V;
            if (dmem_req_tvalid && dmem_req_tready) begin
                if (request_count >= MAX_TRACKED) $fatal(1, "G6_L3_R2_RTL_FAIL request_overflow");
                req_txid[request_count] <= dmem_req_tdata[31:0];
                req_rob[request_count] <= dmem_req_tdata[39:32];
                req_command[request_count] <= dmem_req_tdata[47:40];
                req_size[request_count] <= dmem_req_tdata[55:48];
                req_mask[request_count] <= dmem_req_tdata[63:56];
                req_address[request_count] <= dmem_req_tdata[191:128];
                $display("G6_L3_R2_TRACE config=%0d_%0d case=%0s event=request index=%0d txid=%h rob=%0d outstanding=%0d address=%h",
                         LQ_DEPTH, SQ_DEPTH, current_case, request_count,
                         dmem_req_tdata[31:0], dmem_req_tdata[39:32],
                         dut.state_lsu_outstanding_load_count_V, dmem_req_tdata[191:128]);
                request_count <= request_count + 1;
            end
            if (dmem_resp_tvalid && dmem_resp_tready) begin
                $display("G6_L3_R2_TRACE config=%0d_%0d case=%0s event=response index=%0d txid=%h outstanding=%0d",
                         LQ_DEPTH, SQ_DEPTH, current_case, response_count,
                         dmem_resp_tdata[31:0], dut.state_lsu_outstanding_load_count_V);
                response_count <= response_count + 1;
            end
        end
    end

    function [0:0] lq_valid(input integer slot);
        begin
            case (slot)
                `LQ_VALID_CASE(0)
                `LQ_VALID_CASE(1)
                `LQ_VALID_CASE(2)
                `LQ_VALID_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `LQ_VALID_CASE(4)
                `LQ_VALID_CASE(5)
                `LQ_VALID_CASE(6)
                `LQ_VALID_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `LQ_VALID_CASE(8)
                `LQ_VALID_CASE(9)
                `LQ_VALID_CASE(10)
                `LQ_VALID_CASE(11)
                `LQ_VALID_CASE(12)
                `LQ_VALID_CASE(13)
                `LQ_VALID_CASE(14)
                `LQ_VALID_CASE(15)
`endif
                default: lq_valid = 1'bx;
            endcase
        end
    endfunction

    function [0:0] lq_pending(input integer slot);
        begin
            case (slot)
                `LQ_PENDING_CASE(0)
                `LQ_PENDING_CASE(1)
                `LQ_PENDING_CASE(2)
                `LQ_PENDING_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `LQ_PENDING_CASE(4)
                `LQ_PENDING_CASE(5)
                `LQ_PENDING_CASE(6)
                `LQ_PENDING_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `LQ_PENDING_CASE(8)
                `LQ_PENDING_CASE(9)
                `LQ_PENDING_CASE(10)
                `LQ_PENDING_CASE(11)
                `LQ_PENDING_CASE(12)
                `LQ_PENDING_CASE(13)
                `LQ_PENDING_CASE(14)
                `LQ_PENDING_CASE(15)
`endif
                default: lq_pending = 1'bx;
            endcase
        end
    endfunction

    function [31:0] lq_txid(input integer slot);
        begin
            case (slot)
                `LQ_TXID_CASE(0)
                `LQ_TXID_CASE(1)
                `LQ_TXID_CASE(2)
                `LQ_TXID_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `LQ_TXID_CASE(4)
                `LQ_TXID_CASE(5)
                `LQ_TXID_CASE(6)
                `LQ_TXID_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `LQ_TXID_CASE(8)
                `LQ_TXID_CASE(9)
                `LQ_TXID_CASE(10)
                `LQ_TXID_CASE(11)
                `LQ_TXID_CASE(12)
                `LQ_TXID_CASE(13)
                `LQ_TXID_CASE(14)
                `LQ_TXID_CASE(15)
`endif
                default: lq_txid = 32'hxxxx_xxxx;
            endcase
        end
    endfunction

    function [7:0] lq_rob_idx(input integer slot);
        begin
            case (slot)
                `LQ_ROB_CASE(0)
                `LQ_ROB_CASE(1)
                `LQ_ROB_CASE(2)
                `LQ_ROB_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `LQ_ROB_CASE(4)
                `LQ_ROB_CASE(5)
                `LQ_ROB_CASE(6)
                `LQ_ROB_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `LQ_ROB_CASE(8)
                `LQ_ROB_CASE(9)
                `LQ_ROB_CASE(10)
                `LQ_ROB_CASE(11)
                `LQ_ROB_CASE(12)
                `LQ_ROB_CASE(13)
                `LQ_ROB_CASE(14)
                `LQ_ROB_CASE(15)
`endif
                default: lq_rob_idx = 8'hxx;
            endcase
        end
    endfunction

    function [31:0] lq_alloc(input integer slot);
        begin
            case (slot)
                `LQ_ALLOC_CASE(0)
                `LQ_ALLOC_CASE(1)
                `LQ_ALLOC_CASE(2)
                `LQ_ALLOC_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `LQ_ALLOC_CASE(4)
                `LQ_ALLOC_CASE(5)
                `LQ_ALLOC_CASE(6)
                `LQ_ALLOC_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `LQ_ALLOC_CASE(8)
                `LQ_ALLOC_CASE(9)
                `LQ_ALLOC_CASE(10)
                `LQ_ALLOC_CASE(11)
                `LQ_ALLOC_CASE(12)
                `LQ_ALLOC_CASE(13)
                `LQ_ALLOC_CASE(14)
                `LQ_ALLOC_CASE(15)
`endif
                default: lq_alloc = 32'hxxxx_xxxx;
            endcase
        end
    endfunction

    function [15:0] lq_generation(input integer slot);
        begin
            case (slot)
                `LQ_GEN_CASE(0)
                `LQ_GEN_CASE(1)
                `LQ_GEN_CASE(2)
                `LQ_GEN_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `LQ_GEN_CASE(4)
                `LQ_GEN_CASE(5)
                `LQ_GEN_CASE(6)
                `LQ_GEN_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `LQ_GEN_CASE(8)
                `LQ_GEN_CASE(9)
                `LQ_GEN_CASE(10)
                `LQ_GEN_CASE(11)
                `LQ_GEN_CASE(12)
                `LQ_GEN_CASE(13)
                `LQ_GEN_CASE(14)
                `LQ_GEN_CASE(15)
`endif
                default: lq_generation = 16'hxxxx;
            endcase
        end
    endfunction

    function integer active_lq_count;
        integer i;
        begin
            active_lq_count = 0;
            for (i = 0; i < LQ_DEPTH; i = i + 1)
                if (lq_valid(i) === 1'b1 && lq_pending(i) === 1'b1)
                    active_lq_count = active_lq_count + 1;
        end
    endfunction

    task automatic start_case(input [8*48-1:0] name);
        begin
            current_case = name;
            case_count = case_count + 1;
            $display("G6_L3_R2_CASE_START config=%0d_%0d case=%0s", LQ_DEPTH, SQ_DEPTH, name);
        end
    endtask

    task automatic wait_reset_complete;
        integer guard;
        begin
            guard = 0;
            while (dut.reset_ctrl_completed !== 1'b1 && guard < 40000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 40000)
                $fatal(1, "G6_L3_R2_RTL_FAIL reset_ctrl_timeout fsm=%h state34=%b state35=%b block=%b deadlock=%b",
                       dut.ap_CS_fsm, dut.ap_CS_fsm_state34, dut.ap_CS_fsm_state35,
                       ap_local_block, ap_local_deadlock);
            guard = 0;
            while (dut.ap_CS_fsm_state34 !== 1'b1 && guard < 40000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 40000)
                $fatal(1, "G6_L3_R2_RTL_FAIL reset_idle_timeout fsm=%h reset_done=%b state34=%b state35=%b block=%b deadlock=%b",
                       dut.ap_CS_fsm, dut.reset_ctrl_completed, dut.ap_CS_fsm_state34,
                       dut.ap_CS_fsm_state35, ap_local_block, ap_local_deadlock);
            @(negedge clk);
        end
    endtask

    task automatic reset_core;
        begin
            rst_n = 1'b0;
            dmem_req_tready = 1'b1;
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
            repeat (5) @(negedge clk);
            rst_n = 1'b1;
            wait_reset_complete();
        end
    endtask

    task automatic seed_lq(input integer slot, input [7:0] rob_idx,
                           input [31:0] allocation, input [15:0] generation,
                           input [7:0] branch_mask);
        begin
            case (slot)
                `SEED_LQ_CASE(0)
                `SEED_LQ_CASE(1)
                `SEED_LQ_CASE(2)
                `SEED_LQ_CASE(3)
`ifdef G6_L3_HAS_LQ8
                `SEED_LQ_CASE(4)
                `SEED_LQ_CASE(5)
                `SEED_LQ_CASE(6)
                `SEED_LQ_CASE(7)
`endif
`ifdef G6_L3_HAS_LQ16
                `SEED_LQ_CASE(8)
                `SEED_LQ_CASE(9)
                `SEED_LQ_CASE(10)
                `SEED_LQ_CASE(11)
                `SEED_LQ_CASE(12)
                `SEED_LQ_CASE(13)
                `SEED_LQ_CASE(14)
                `SEED_LQ_CASE(15)
`endif
                default: $fatal(1, "G6_L3_R2_RTL_FAIL bad_lq_slot %0d depth=%0d", slot, LQ_DEPTH);
            endcase
        end
    endtask

    task automatic seed_rob_load_only(input [7:0] rob_idx, input integer lq_idx,
                                      input [31:0] allocation, input [15:0] generation,
                                      input [63:0] address, input [4:0] tail_after);
        begin
            dut.state_rob_tail = tail_after;
            dut.state_rob_maybe_full = 1'b0;
            dut.state_rob_entries_valid_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_busy_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_exception_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_memory_valid_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_is_load_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_is_store_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_signed_load_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_memory_request_sent_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_memory_completed_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_memory_address_U.ram[rob_idx] = address;
            dut.state_rob_entries_memory_data_U.ram[rob_idx] = 64'd0;
            dut.state_rob_entries_memory_mask_U.ram[rob_idx] = 8'hff;
            dut.state_rob_entries_memory_size_U.ram[rob_idx] = 8'd3;
            dut.state_rob_entries_uop_iq_type_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_uop_imm_packed_1_U.ram[rob_idx] = lq_idx[7:0];
            dut.state_rob_entries_uop_imm_packed_2_U.ram[rob_idx] = rob_idx[4:0];
            dut.state_rob_entries_uop_imm_packed_3_U.ram[rob_idx] = generation;
            dut.state_rob_entries_uop_imm_packed_4_U.ram[rob_idx] = allocation;
            dut.state_rob_entries_uop_iw_p1_poisoned_1_U.ram[rob_idx] = 8'd0;
        end
    endtask

    task automatic seed_load(input [7:0] rob_idx, input integer lq_idx,
                             input [31:0] allocation, input [15:0] generation,
                             input [63:0] address, input [4:0] tail_after);
        begin
            seed_rob_load_only(rob_idx, lq_idx, allocation, generation, address, tail_after);
            seed_lq(lq_idx, rob_idx, allocation, generation, 8'd0);
        end
    endtask

    task automatic seed_store0(input [7:0] rob_idx, input [31:0] allocation,
                               input [15:0] generation, input [63:0] address,
                               input [63:0] data, input [7:0] mask,
                               input [0:0] busy);
        begin
            dut.state_rob_head = 5'd0;
            dut.state_rob_entries_valid_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_busy_U.ram[rob_idx] = busy;
            dut.state_rob_entries_exception_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_memory_valid_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_is_load_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_is_store_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_memory_request_sent_U.ram[rob_idx] = 1'b0;
            dut.state_rob_entries_memory_completed_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_memory_address_U.ram[rob_idx] = address;
            dut.state_rob_entries_memory_data_U.ram[rob_idx] = data;
            dut.state_rob_entries_memory_mask_U.ram[rob_idx] = mask;
            dut.state_rob_entries_memory_size_U.ram[rob_idx] = 8'd3;
            dut.state_rob_entries_uop_iq_type_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_uop_imm_packed_5_U.ram[rob_idx] = 8'd0;
            dut.state_rob_entries_uop_imm_packed_2_U.ram[rob_idx] = rob_idx[4:0];
            dut.state_rob_entries_uop_imm_packed_U.ram[rob_idx] = generation;
            dut.state_rob_entries_uop_imm_packed_4_U.ram[rob_idx] = allocation;
            dut.state_lsu_stq_count_V = 5'd1;
            `G6_L3_STQ_VALID0 = 1'b1;
            `G6_L3_STQ_ADDR_VALID0 = 1'b1;
            `G6_L3_STQ_DATA_VALID0 = 1'b1;
            `G6_L3_STQ_ROB0 = rob_idx;
            `G6_L3_STQ_MASK0 = mask;
            `G6_L3_STQ_ALLOC0 = allocation;
            `G6_L3_STQ_GEN0 = generation;
            `G6_L3_STQ_ADDR0 = address;
            `G6_L3_STQ_DATA0 = data;
        end
    endtask

    task automatic wait_requests(input integer target);
        integer guard;
        begin
            guard = 0;
            while (request_count < target && guard < 80000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 80000)
                $fatal(1, "G6_L3_R2_RTL_FAIL request_timeout target=%0d got=%0d case=%0s fsm=%h block=%b deadlock=%b",
                       target, request_count, current_case, dut.ap_CS_fsm, ap_local_block, ap_local_deadlock);
            @(negedge clk);
        end
    endtask

    task automatic wait_architectural_cycles(input integer count);
        integer n;
        integer guard;
        begin
            for (n = 0; n < count; n = n + 1) begin
                guard = 0;
                while (dut.ap_CS_fsm_state34 === 1'b1 && guard < 80000) begin
                    @(posedge clk);
                    guard = guard + 1;
                end
                if (guard >= 80000) $fatal(1, "G6_L3_R2_RTL_FAIL arch_leave_timeout case=%0s", current_case);
                guard = 0;
                while (dut.ap_CS_fsm_state34 !== 1'b1 && guard < 80000) begin
                    @(posedge clk);
                    guard = guard + 1;
                end
                if (guard >= 80000) $fatal(1, "G6_L3_R2_RTL_FAIL arch_return_timeout case=%0s", current_case);
            end
            @(negedge clk);
        end
    endtask

    task automatic expect_request(input integer index, input [31:0] txid,
                                  input [7:0] rob_idx, input [63:0] address);
        begin
            if (req_txid[index] !== txid || req_rob[index] !== rob_idx ||
                req_command[index] !== 8'd0 || req_size[index] !== 8'd3 ||
                req_mask[index] !== 8'hff || req_address[index] !== address) begin
                $display("G6_L3_R2_DIAG request[%0d] txid=%h rob=%h cmd=%h size=%h mask=%h address=%h",
                         index, req_txid[index], req_rob[index], req_command[index],
                         req_size[index], req_mask[index], req_address[index]);
                $fatal(1, "G6_L3_R2_RTL_FAIL request_mismatch index=%0d case=%0s", index, current_case);
            end
        end
    endtask

    task automatic expect_lq_pending(input integer slot, input [31:0] txid,
                                     input [7:0] rob_idx, input [31:0] allocation,
                                     input [15:0] generation);
        begin
            if (lq_valid(slot) !== 1'b1 || lq_pending(slot) !== 1'b1 ||
                lq_txid(slot) !== txid || lq_rob_idx(slot) !== rob_idx ||
                lq_alloc(slot) !== allocation || lq_generation(slot) !== generation)
                $fatal(1, "G6_L3_R2_RTL_FAIL lq_owner slot=%0d txid=%h valid=%b pending=%b got_txid=%h rob=%h alloc=%h gen=%h case=%0s",
                       slot, txid, lq_valid(slot), lq_pending(slot), lq_txid(slot),
                       lq_rob_idx(slot), lq_alloc(slot), lq_generation(slot), current_case);
        end
    endtask

    task automatic expect_lq_reclaimed(input integer slot);
        begin
            if (lq_valid(slot) !== 1'b0 || lq_pending(slot) !== 1'b0)
                $fatal(1, "G6_L3_R2_RTL_FAIL lq_not_reclaimed slot=%0d valid=%b pending=%b txid=%h case=%0s",
                       slot, lq_valid(slot), lq_pending(slot), lq_txid(slot), current_case);
        end
    endtask

    task automatic expect_unique_active_txids;
        integer i, j;
        begin
            for (i = 0; i < LQ_DEPTH; i = i + 1) begin
                if (!(lq_valid(i) === 1'b1 && lq_pending(i) === 1'b1)) continue;
                for (j = i + 1; j < LQ_DEPTH; j = j + 1) begin
                    if (lq_valid(j) === 1'b1 && lq_pending(j) === 1'b1 && lq_txid(i) === lq_txid(j))
                        $fatal(1, "G6_L3_R2_RTL_FAIL duplicate_active_txid slot_i=%0d slot_j=%0d txid=%h case=%0s",
                               i, j, lq_txid(i), current_case);
                end
            end
        end
    endtask

    task automatic send_response(input [31:0] txid, input [63:0] data);
        integer guard;
        begin
            @(negedge clk);
            dmem_resp_tdata = 384'd0;
            dmem_resp_tdata[31:0] = txid;
            dmem_resp_tdata[127:64] = data;
            dmem_resp_tdata[191:128] = data;
            dmem_resp_tvalid = 1'b1;
            guard = 0;
            while (dmem_resp_tready !== 1'b1 && guard < 80000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 80000) $fatal(1, "G6_L3_R2_RTL_FAIL response_ready_timeout txid=%h case=%0s", txid, current_case);
            @(posedge clk);
            @(negedge clk);
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
        end
    endtask

    task automatic send_response_count_block(input [31:0] txid, input [63:0] data,
                                             output integer blocked_cycles);
        integer guard;
        begin
            @(negedge clk);
            dmem_resp_tdata = 384'd0;
            dmem_resp_tdata[31:0] = txid;
            dmem_resp_tdata[127:64] = data;
            dmem_resp_tdata[191:128] = data;
            dmem_resp_tvalid = 1'b1;
            guard = 0;
            blocked_cycles = 0;
            while (dmem_resp_tready !== 1'b1 && guard < 80000) begin
                @(posedge clk);
                blocked_cycles = blocked_cycles + 1;
                guard = guard + 1;
            end
            if (guard >= 80000) $fatal(1, "G6_L3_R2_RTL_FAIL response_bp_timeout txid=%h case=%0s", txid, current_case);
            @(posedge clk);
            @(negedge clk);
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
            response_backpressure_blocked_cycles = response_backpressure_blocked_cycles + blocked_cycles;
        end
    endtask

    task automatic wait_load_done(input [7:0] rob_idx, input integer lq_idx,
                                  input [63:0] expected_data);
        integer guard;
        begin
            guard = 0;
            while ((dut.state_rob_entries_memory_completed_U.ram[rob_idx] !== 1'b1 ||
                    lq_valid(lq_idx) !== 1'b0 || lq_pending(lq_idx) !== 1'b0) &&
                   guard < 80000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 80000)
                $fatal(1, "G6_L3_R2_RTL_FAIL completion_timeout rob=%0d lq=%0d completed=%b valid=%b pending=%b txid=%h case=%0s",
                       rob_idx, lq_idx, dut.state_rob_entries_memory_completed_U.ram[rob_idx],
                       lq_valid(lq_idx), lq_pending(lq_idx), lq_txid(lq_idx), current_case);
            @(negedge clk);
            if (dut.state_rob_entries_busy_U.ram[rob_idx] !== 1'b0)
                $fatal(1, "G6_L3_R2_RTL_FAIL rob_busy_after_completion rob=%0d case=%0s", rob_idx, current_case);
            if (dut.state_rob_entries_memory_data_U.ram[rob_idx] !== expected_data)
                $fatal(1, "G6_L3_R2_RTL_FAIL load_value rob=%0d got=%h expected=%h case=%0s",
                       rob_idx, dut.state_rob_entries_memory_data_U.ram[rob_idx], expected_data, current_case);
            expect_lq_reclaimed(lq_idx);
            $display("G6_L3_R2_TRACE config=%0d_%0d case=%0s event=complete rob=%0d lq=%0d data=%h outstanding=%0d",
                     LQ_DEPTH, SQ_DEPTH, current_case, rob_idx, lq_idx, expected_data,
                     dut.state_lsu_outstanding_load_count_V);
        end
    endtask

    task automatic wait_no_new_requests(input integer base_count, input integer clock_count,
                                        input [8*48-1:0] check_name);
        begin
            repeat (clock_count) @(posedge clk);
            if (request_count !== base_count)
                $fatal(1, "G6_L3_R2_RTL_FAIL unexpected_request check=%0s base=%0d got=%0d case=%0s",
                       check_name, base_count, request_count, current_case);
            @(negedge clk);
        end
    endtask

    task automatic count_requests(input integer start_idx, input integer end_idx,
                                  output integer load_reqs, output integer store_reqs);
        integer i;
        begin
            load_reqs = 0;
            store_reqs = 0;
            for (i = start_idx; i < end_idx; i = i + 1) begin
                if (req_command[i] === 8'd0)
                    load_reqs = load_reqs + 1;
                else if (req_command[i] === 8'd1)
                    store_reqs = store_reqs + 1;
            end
        end
    endtask

    task automatic expect_store_forwarding_seed(input [7:0] store_rob,
                                                input [31:0] store_alloc,
                                                input [15:0] store_generation,
                                                input [63:0] store_addr,
                                                input [63:0] store_data,
                                                input [7:0] store_mask,
                                                input [7:0] load_rob,
                                                input integer load_lq,
                                                input [31:0] load_alloc,
                                                input [15:0] load_generation,
                                                input [63:0] load_addr);
        begin
            if (store_rob >= load_rob)
                $fatal(1, "G6_L3_R2_RTL_FAIL forwarding_rob_order store=%0d load=%0d", store_rob, load_rob);
            if (dut.state_rob_entries_valid_U.ram[store_rob] !== 1'b1 ||
                dut.state_rob_entries_is_store_U.ram[store_rob] !== 1'b1 ||
                dut.state_rob_entries_memory_valid_U.ram[store_rob] !== 1'b1)
                $fatal(1, "G6_L3_R2_RTL_FAIL forwarding_older_store_invalid rob=%0d", store_rob);
            if (`G6_L3_STQ_VALID0 !== 1'b1 || `G6_L3_STQ_ADDR_VALID0 !== 1'b1 ||
                `G6_L3_STQ_DATA_VALID0 !== 1'b1 || `G6_L3_STQ_ROB0 !== store_rob ||
                `G6_L3_STQ_ALLOC0 !== store_alloc || `G6_L3_STQ_GEN0 !== store_generation ||
                `G6_L3_STQ_ADDR0 !== store_addr || `G6_L3_STQ_DATA0 !== store_data ||
                `G6_L3_STQ_MASK0 !== store_mask)
                $fatal(1, "G6_L3_R2_RTL_FAIL forwarding_sq_owner_tuple valid=%b addr_valid=%b data_valid=%b rob=%h alloc=%h gen=%h addr=%h data=%h mask=%h",
                       `G6_L3_STQ_VALID0, `G6_L3_STQ_ADDR_VALID0, `G6_L3_STQ_DATA_VALID0,
                       `G6_L3_STQ_ROB0, `G6_L3_STQ_ALLOC0, `G6_L3_STQ_GEN0,
                       `G6_L3_STQ_ADDR0, `G6_L3_STQ_DATA0, `G6_L3_STQ_MASK0);
            if (dut.state_rob_entries_valid_U.ram[load_rob] !== 1'b1 ||
                dut.state_rob_entries_is_load_U.ram[load_rob] !== 1'b1 ||
                dut.state_rob_entries_memory_valid_U.ram[load_rob] !== 1'b1 ||
                dut.state_rob_entries_memory_address_U.ram[load_rob] !== load_addr ||
                dut.state_rob_entries_memory_mask_U.ram[load_rob] !== 8'hff ||
                dut.state_rob_entries_memory_size_U.ram[load_rob] !== 8'd3 ||
                lq_valid(load_lq) !== 1'b1 || lq_pending(load_lq) !== 1'b0 ||
                lq_rob_idx(load_lq) !== load_rob || lq_alloc(load_lq) !== load_alloc ||
                lq_generation(load_lq) !== load_generation)
                $fatal(1, "G6_L3_R2_RTL_FAIL forwarding_younger_load_invalid rob=%0d lq=%0d", load_rob, load_lq);
        end
    endtask

    task automatic send_stale_response_probe(input [31:0] txid, input [63:0] data,
                                             input [7:0] rob0, input [7:0] rob1,
                                             output integer accepted,
                                             output integer wrong_completion,
                                             output integer wrong_prf_writeback);
        integer i;
        begin
            accepted = 0;
            wrong_completion = 0;
            wrong_prf_writeback = 0;
            @(negedge clk);
            dmem_resp_tdata = 384'd0;
            dmem_resp_tdata[31:0] = txid;
            dmem_resp_tdata[127:64] = data;
            dmem_resp_tdata[191:128] = data;
            dmem_resp_tvalid = 1'b1;
            for (i = 0; i < 40; i = i + 1) begin
                @(posedge clk); #1;
                if (dmem_resp_tready === 1'b1)
                    accepted = 1;
                if (dut.state_rob_entries_memory_completed_U.ram[rob0] === 1'b1 ||
                    dut.state_rob_entries_memory_completed_U.ram[rob1] === 1'b1 ||
                    dut.state_completion_load_response_valid === 1'b1)
                    wrong_completion = 1;
                if (dut.state_completion_writebacks_valid_0 === 1'b1 ||
                    dut.state_completion_writebacks_valid_1 === 1'b1 ||
                    dut.state_int_rf_bank0_we0 === 1'b1 ||
                    dut.state_int_rf_bank1_we0 === 1'b1)
                    wrong_prf_writeback = 1;
            end
            @(negedge clk);
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
        end
    endtask

    task automatic run_saturation_reuse_stale;
        integer i;
        integer base_req;
        integer base_resp;
        integer mid;
        begin
            start_case("max_saturation_reuse_stale");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_tail_V = 5'd0;
            dut.state_lsu_ldq_count_V = LQ_DEPTH;
            dut.state_lsu_outstanding_load_count_V = 5'd0;
            dut.state_lsu_next_transaction_id = TX_BASE;
            dut.state_completion_load_response_valid = 1'b0;
            for (i = 0; i < LQ_DEPTH; i = i + 1) begin
                slot_alloc[i] = 32'h0000_1000 + i;
                slot_generation[i] = 16'h0100 + i;
                slot_data[i] = DATA_BASE + i;
                seed_load(i, i, slot_alloc[i], slot_generation[i],
                          ADDR_BASE + (i * 64'd8), LQ_DEPTH);
            end
            base_req = request_count;
            base_resp = response_count;
            wait_requests(base_req + LQ_DEPTH);
            for (i = 0; i < LQ_DEPTH; i = i + 1) begin
                slot_txid[i] = TX_BASE + i;
                expect_request(base_req + i, TX_BASE + i, i, ADDR_BASE + (i * 64'd8));
                expect_lq_pending(i, TX_BASE + i, i, slot_alloc[i], slot_generation[i]);
            end
            expect_unique_active_txids();
            if (dut.state_lsu_outstanding_load_count_V != LQ_DEPTH || active_lq_count() != LQ_DEPTH)
                $fatal(1, "G6_L3_R2_RTL_FAIL saturation_count got=%0d active=%0d expected=%0d",
                       dut.state_lsu_outstanding_load_count_V, active_lq_count(), LQ_DEPTH);
            seed_rob_load_only(8'd31, 0, 32'h0000_1fff, 16'h01ff,
                               ADDR_BASE + 64'h1000, 5'd31);
            wait_no_new_requests(base_req + LQ_DEPTH, 400, "saturated_extra_blocked");
            $display("G6_L3_R2_CASE_PASS case=outstanding_slot_saturation max=%0d", LQ_DEPTH);

            mid = LQ_DEPTH / 2;
            send_response(slot_txid[mid], slot_data[mid]);
            wait_load_done(mid, mid, slot_data[mid]);
            if (dut.state_lsu_outstanding_load_count_V !== (LQ_DEPTH - 1))
                $fatal(1, "G6_L3_R2_RTL_FAIL count_after_first_response got=%0d expected=%0d",
                       dut.state_lsu_outstanding_load_count_V, LQ_DEPTH - 1);
            $display("G6_L3_R2_CASE_PASS case=arbitrary_out_of_order_first slot=%0d txid=%h", mid, slot_txid[mid]);

            slot_alloc[mid] = 32'h0000_2000 + mid;
            slot_generation[mid] = 16'h0200 + mid;
            slot_data[mid] = DATA_BASE + 64'h100 + mid;
            seed_load(mid, mid, slot_alloc[mid], slot_generation[mid],
                      ADDR_BASE + 64'h2000 + (mid * 64'd8), (LQ_DEPTH + 1));
            dut.state_lsu_ldq_count_V = LQ_DEPTH;
            wait_requests(base_req + LQ_DEPTH + 1);
            slot_txid[mid] = TX_BASE + LQ_DEPTH;
            expect_request(base_req + LQ_DEPTH, slot_txid[mid], mid,
                           ADDR_BASE + 64'h2000 + (mid * 64'd8));
            expect_lq_pending(mid, slot_txid[mid], mid, slot_alloc[mid], slot_generation[mid]);
            $display("G6_L3_R2_CASE_PASS case=lq_release_immediate_reuse old_txid=%h new_txid=%h",
                     TX_BASE + mid, slot_txid[mid]);

            send_response(TX_BASE + mid, 64'hfeed_face_cafe_beef);
            wait_architectural_cycles(2);
            expect_lq_pending(mid, slot_txid[mid], mid, slot_alloc[mid], slot_generation[mid]);
            if (dut.state_rob_entries_memory_completed_U.ram[mid] !== 1'b0)
                $fatal(1, "G6_L3_R2_RTL_FAIL stale_old_txid_completed_new_owner");
            if (dut.state_lsu_outstanding_load_count_V != LQ_DEPTH)
                $fatal(1, "G6_L3_R2_RTL_FAIL stale_old_txid_count got=%0d expected=%0d",
                       dut.state_lsu_outstanding_load_count_V, LQ_DEPTH);
            $display("G6_L3_R2_CASE_PASS case=stale_response_after_lq_reuse rejected_txid=%h owner_txid=%h",
                     TX_BASE + mid, slot_txid[mid]);

            if ((LQ_DEPTH - 1) != mid) begin
                send_response(slot_txid[LQ_DEPTH - 1], slot_data[LQ_DEPTH - 1]);
                wait_load_done((LQ_DEPTH - 1), LQ_DEPTH - 1, slot_data[LQ_DEPTH - 1]);
            end
            if (mid != 0) begin
                send_response(slot_txid[0], slot_data[0]);
                wait_load_done(8'd0, 0, slot_data[0]);
            end
            send_response(slot_txid[mid], slot_data[mid]);
            wait_load_done(mid, mid, slot_data[mid]);
            for (i = LQ_DEPTH - 2; i >= 1; i = i - 1) begin
                if (i != mid) begin
                    send_response(slot_txid[i], slot_data[i]);
                    wait_load_done(i, i, slot_data[i]);
                end
            end
            if (dut.state_lsu_outstanding_load_count_V !== 0 || active_lq_count() != 0)
                $fatal(1, "G6_L3_R2_RTL_FAIL final_drain count=%0d active=%0d",
                       dut.state_lsu_outstanding_load_count_V, active_lq_count());
            if ((request_count - base_req) !== (LQ_DEPTH + 1) ||
                (response_count - base_resp) !== (LQ_DEPTH + 2))
                $fatal(1, "G6_L3_R2_RTL_FAIL saturation_counts requests=%0d responses=%0d expected_req=%0d expected_resp=%0d",
                       request_count - base_req, response_count - base_resp,
                       LQ_DEPTH + 1, LQ_DEPTH + 2);
            $display("G6_L3_R2_CASE_PASS case=max_saturation_reuse_stale requests=%0d responses=%0d max=%0d",
                     request_count - base_req, response_count - base_resp, LQ_DEPTH);
        end
    endtask

    task automatic run_txid_wrap_duplicate_skip;
        integer base_req;
        integer base_resp;
        reg [31:0] first_tx;
        begin
            start_case("txid_wrap_duplicate_skip");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_count_V = 3;
            dut.state_lsu_next_transaction_id = 32'hffff_fffe;
            seed_load(8'd0, 0, 32'h3000, 16'h0300, ADDR_BASE + 64'h3000, 5'd3);
            seed_load(8'd1, 1, 32'h3001, 16'h0301, ADDR_BASE + 64'h3008, 5'd3);
            seed_load(8'd2, 2, 32'h3002, 16'h0302, ADDR_BASE + 64'h3010, 5'd3);
            base_req = request_count;
            base_resp = response_count;
            wait_requests(base_req + 3);
            expect_request(base_req + 0, 32'hffff_fffe, 8'd0, ADDR_BASE + 64'h3000);
            expect_request(base_req + 1, 32'hffff_ffff, 8'd1, ADDR_BASE + 64'h3008);
            expect_request(base_req + 2, 32'h0000_0000, 8'd2, ADDR_BASE + 64'h3010);
            expect_unique_active_txids();
            send_response(32'h0000_0000, DATA_BASE + 64'h3002);
            wait_load_done(8'd2, 2, DATA_BASE + 64'h3002);
            send_response(32'h0000_0000, 64'hdead_dead_dead_dead);
            wait_architectural_cycles(2);
            if (dut.state_lsu_outstanding_load_count_V !== 2)
                $fatal(1, "G6_L3_R2_RTL_FAIL duplicate_response_changed_count got=%0d",
                       dut.state_lsu_outstanding_load_count_V);
            send_response(32'hffff_fffe, DATA_BASE + 64'h3000);
            wait_load_done(8'd0, 0, DATA_BASE + 64'h3000);
            send_response(32'hffff_ffff, DATA_BASE + 64'h3001);
            wait_load_done(8'd1, 1, DATA_BASE + 64'h3001);
            if ((request_count - base_req) !== 3 || (response_count - base_resp) !== 4)
                $fatal(1, "G6_L3_R2_RTL_FAIL wrap_counts requests=%0d responses=%0d",
                       request_count - base_req, response_count - base_resp);

            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_count_V = 2;
            dut.state_lsu_next_transaction_id = 32'h42;
            seed_load(8'd0, 0, 32'h3100, 16'h0310, ADDR_BASE + 64'h3100, 5'd2);
            seed_load(8'd1, 1, 32'h3101, 16'h0311, ADDR_BASE + 64'h3108, 5'd2);
            base_req = request_count;
            wait_requests(base_req + 1);
            first_tx = req_txid[base_req];
            dut.state_lsu_next_transaction_id = first_tx;
            wait_requests(base_req + 2);
            if (req_txid[base_req + 1] === first_tx)
                $fatal(1, "G6_L3_R2_RTL_FAIL active_txid_duplicate_not_skipped txid=%h", first_tx);
            expect_unique_active_txids();
            $display("G6_L3_R2_CASE_PASS case=txid_wrap_duplicate_skip first=%h second=%h", first_tx, req_txid[base_req + 1]);
        end
    endtask

    task automatic run_request_backpressure;
        integer base_req;
        integer i;
        integer top_valid_seen;
        integer top_drop;
        integer payload_change;
        integer handshakes_while_blocked;
        integer internal_handshakes;
        integer release_base;
        integer load_reqs;
        integer store_reqs;
        reg [319:0] held_req;
        integer local_fail;
        begin
            start_case("request_backpressure");
            local_fail = 0;
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_count_V = 1;
            dut.state_lsu_next_transaction_id = 32'h5000;
            seed_load(8'd0, 0, 32'h5000, 16'h0500, ADDR_BASE + 64'h5000, 5'd1);
            base_req = request_count;
            dmem_req_tready = 1'b0;
            top_valid_seen = 0;
            top_drop = 0;
            payload_change = 0;
            handshakes_while_blocked = 0;
            internal_handshakes = 0;
            held_req = 320'd0;
            for (i = 0; i < 1200; i = i + 1) begin
                @(posedge clk); #1;
                if (`G6_L3_INTERNAL_DMEM_TVALID && `G6_L3_INTERNAL_DMEM_TREADY)
                    internal_handshakes = internal_handshakes + 1;
                if (dmem_req_tvalid && dmem_req_tready)
                    handshakes_while_blocked = handshakes_while_blocked + 1;
                if (!top_valid_seen && dmem_req_tvalid) begin
                    top_valid_seen = 1;
                    held_req = dmem_req_tdata;
                end else if (top_valid_seen && dmem_req_tready === 1'b0) begin
                    if (dmem_req_tvalid !== 1'b1)
                        top_drop = top_drop + 1;
                    else if (dmem_req_tdata !== held_req)
                        payload_change = payload_change + 1;
                end
            end
            if (request_count !== base_req || handshakes_while_blocked != 0) begin
                local_fail = 1;
                $display("G6_L3_R2_CASE_FAIL case=request_backpressure classification=INTERFACE_PROTOCOL_ERROR reason=handshake_while_tready_low base=%0d got=%0d",
                         base_req, request_count);
            end else if (top_valid_seen != 1) begin
                local_fail = 1;
                $display("G6_L3_R2_CASE_FAIL case=request_backpressure classification=INTERFACE_PROTOCOL_ERROR reason=no_external_tvalid_after_owner_reserved sent=%b outstanding=%0d internal_handshakes=%0d",
                          dut.state_rob_entries_memory_request_sent_U.ram[0],
                          dut.state_lsu_outstanding_load_count_V, internal_handshakes);
            end else if (top_drop != 0 || payload_change != 0) begin
                local_fail = 1;
                $display("G6_L3_R2_CASE_FAIL case=request_backpressure classification=INTERFACE_PROTOCOL_ERROR reason=request_not_stable base=%0d got=%0d top_drop=%0d payload_change=%0d",
                         base_req, request_count, top_drop, payload_change);
            end
            if (local_fail != 0) begin
                case_failures = case_failures + 1;
                dmem_req_tready = 1'b1;
                reset_core();
            end else begin
                expect_lq_pending(0, 32'h5000, 8'd0, 32'h5000, 16'h0500);
                if (dut.state_lsu_outstanding_load_count_V !== 1)
                    $fatal(1, "G6_L3_R2_RTL_FAIL req_bp_owner_count got=%0d",
                           dut.state_lsu_outstanding_load_count_V);
                if (held_req[31:0] !== 32'h5000 || held_req[39:32] !== 8'd0 ||
                    held_req[47:40] !== 8'd0 || held_req[191:128] !== ADDR_BASE + 64'h5000)
                    $fatal(1, "G6_L3_R2_RTL_FAIL req_bp_held_payload txid=%h rob=%h cmd=%h addr=%h",
                           held_req[31:0], held_req[39:32], held_req[47:40], held_req[191:128]);
                release_base = request_count;
                dmem_req_tready = 1'b1;
                wait_requests(release_base + 1);
                if ((request_count - release_base) !== 1)
                    $fatal(1, "G6_L3_R2_RTL_FAIL req_bp_duplicate_or_loss release_base=%0d got=%0d",
                           release_base, request_count);
                count_requests(release_base, request_count, load_reqs, store_reqs);
                if (load_reqs !== 1 || store_reqs !== 0)
                    $fatal(1, "G6_L3_R2_RTL_FAIL req_bp_wrong_command load_reqs=%0d store_reqs=%0d",
                           load_reqs, store_reqs);
                expect_request(release_base, 32'h5000, 8'd0, ADDR_BASE + 64'h5000);
                expect_lq_pending(0, 32'h5000, 8'd0, 32'h5000, 16'h0500);
                send_response(32'h5000, DATA_BASE + 64'h5000);
                wait_load_done(8'd0, 0, DATA_BASE + 64'h5000);
                $display("G6_L3_R2_CASE_PASS case=request_backpressure owner_reserved_before_external_handshake=%0b tvalid_hold=1 payload_stable=1 handshakes=%0d load_reqs=%0d store_reqs=%0d",
                         dut.state_rob_entries_memory_request_sent_U.ram[0], request_count - release_base,
                         load_reqs, store_reqs);
            end
        end
    endtask

    task automatic run_response_backpressure_and_consecutive;
        integer base_req;
        integer blocked;
        begin
            start_case("response_backpressure_consecutive");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_count_V = 2;
            dut.state_lsu_next_transaction_id = 32'h6000;
            seed_load(8'd0, 0, 32'h6000, 16'h0600, ADDR_BASE + 64'h6000, 5'd2);
            seed_load(8'd1, 1, 32'h6001, 16'h0601, ADDR_BASE + 64'h6008, 5'd2);
            base_req = request_count;
            wait_requests(base_req + 2);
            send_response(32'h6000, DATA_BASE + 64'h6000);
            send_response_count_block(32'h6001, DATA_BASE + 64'h6001, blocked);
            wait_load_done(8'd0, 0, DATA_BASE + 64'h6000);
            wait_load_done(8'd1, 1, DATA_BASE + 64'h6001);
            $display("G6_L3_R2_CASE_PASS case=response_backpressure_consecutive blocked_cycles=%0d", blocked);
        end
    endtask

    task automatic run_store_forwarding;
        integer base_req;
        integer i;
        integer load_reqs;
        integer store_reqs;
        integer outstanding_before;
        begin
            start_case("store_forwarding");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_rob_tail = 5'd2;
            dut.state_rob_maybe_full = 1'b0;
            seed_store0(8'd0, 32'h7000, 16'h0700, ADDR_BASE + 64'h7000,
                        64'h1234_5678_9abc_def0, 8'hff, 1'b1);
            dut.state_lsu_ldq_count_V = 1;
            seed_load(8'd1, 0, 32'h7001, 16'h0701, ADDR_BASE + 64'h7000, 5'd2);
            expect_store_forwarding_seed(8'd0, 32'h7000, 16'h0700, ADDR_BASE + 64'h7000,
                                         64'h1234_5678_9abc_def0, 8'hff,
                                         8'd1, 0, 32'h7001, 16'h0701,
                                         ADDR_BASE + 64'h7000);
            base_req = request_count;
            outstanding_before = dut.state_lsu_outstanding_load_count_V;
            for (i = 0; i < 300; i = i + 1) begin
                @(posedge clk); #1;
                if (request_count !== base_req ||
                    dut.state_rob_entries_memory_completed_U.ram[1] === 1'b1)
                    i = 300;
            end
            count_requests(base_req, request_count, load_reqs, store_reqs);
            if (load_reqs !== 0 || store_reqs !== 0 || request_count !== base_req)
                $fatal(1, "G6_L3_R2_RTL_FAIL forwarding_external_request load_reqs=%0d store_reqs=%0d base=%0d got=%0d",
                       load_reqs, store_reqs, base_req, request_count);
            wait_load_done(8'd1, 0, 64'h1234_5678_9abc_def0);
            if (dut.state_lsu_outstanding_load_count_V !== outstanding_before ||
                dut.state_lsu_outstanding_load_count_V !== 0)
                $fatal(1, "G6_L3_R2_RTL_FAIL forwarding_outstanding_count before=%0d got=%0d",
                       outstanding_before, dut.state_lsu_outstanding_load_count_V);
            $display("G6_L3_R2_CASE_PASS case=store_forwarding older_store_valid=1 younger_load_valid=1 rob_order=1 sq_owner_match=1 full_cover=1 load_data=%h external_loads=%0d external_stores=%0d outstanding_delta=0",
                     dut.state_rob_entries_memory_data_U.ram[1], load_reqs, store_reqs);

            start_case("store_forwarding_partial_negative");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_rob_tail = 5'd2;
            dut.state_rob_maybe_full = 1'b0;
            dut.state_lsu_next_transaction_id = 32'h7100;
            seed_store0(8'd0, 32'h7100, 16'h0710, ADDR_BASE + 64'h7100,
                        64'h0bad_cafe_1234_5678, 8'h0f, 1'b1);
            dut.state_lsu_ldq_count_V = 1;
            seed_load(8'd1, 0, 32'h7101, 16'h0711, ADDR_BASE + 64'h7100, 5'd2);
            expect_store_forwarding_seed(8'd0, 32'h7100, 16'h0710, ADDR_BASE + 64'h7100,
                                         64'h0bad_cafe_1234_5678, 8'h0f,
                                         8'd1, 0, 32'h7101, 16'h0711,
                                         ADDR_BASE + 64'h7100);
            base_req = request_count;
            for (i = 0; i < 500; i = i + 1) begin
                @(posedge clk); #1;
                if (request_count !== base_req ||
                    dut.state_rob_entries_memory_completed_U.ram[1] === 1'b1)
                    i = 500;
            end
            count_requests(base_req, request_count, load_reqs, store_reqs);
            if (dut.state_rob_entries_memory_completed_U.ram[1] === 1'b1 && request_count === base_req)
                $fatal(1, "G6_L3_R2_RTL_FAIL partial_cover_forwarding_only data=%h",
                       dut.state_rob_entries_memory_data_U.ram[1]);
            if (load_reqs > 1 || store_reqs != 0)
                $fatal(1, "G6_L3_R2_RTL_FAIL partial_forwarding_unexpected_requests load=%0d store=%0d",
                       load_reqs, store_reqs);
            if (load_reqs == 1) begin
                expect_request(base_req, 32'h7100, 8'd1, ADDR_BASE + 64'h7100);
                send_response(32'h7100, 64'hfeed_7100_feed_7100);
                wait_load_done(8'd1, 0, 64'hfeed_7100_feed_7100);
            end
            $display("G6_L3_R2_CASE_PASS case=store_forwarding_partial_negative partial_cover=1 forwarding_only=0 external_loads=%0d external_stores=%0d load_completed=%0b load_data=%h",
                     load_reqs, store_reqs,
                     dut.state_rob_entries_memory_completed_U.ram[1],
                     dut.state_rob_entries_memory_data_U.ram[1]);
            reset_core();
        end
    endtask

    task automatic run_flush_with_pending;
        integer base_req;
        integer base_resp;
        integer i;
        integer exception_seen;
        integer redirect_seen;
        integer flush_seen;
        integer stale_accepted;
        integer wrong_completion;
        integer wrong_prf_writeback;
        begin
            start_case("flush_with_pending");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_rob_tail = 5'd2;
            dut.state_rob_maybe_full = 1'b0;
            dut.state_lsu_ldq_count_V = 2;
            dut.state_lsu_next_transaction_id = 32'h8000;
            seed_load(8'd0, 0, 32'h8000, 16'h0800, ADDR_BASE + 64'h8000, 5'd2);
            seed_load(8'd1, 1, 32'h8001, 16'h0801, ADDR_BASE + 64'h8008, 5'd2);
            base_req = request_count;
            wait_requests(base_req + 2);
            expect_lq_pending(0, 32'h8000, 8'd0, 32'h8000, 16'h0800);
            expect_lq_pending(1, 32'h8001, 8'd1, 32'h8001, 16'h0801);
            if (dut.state_lsu_outstanding_load_count_V !== 2 || active_lq_count() != 2)
                $fatal(1, "G6_L3_R2_RTL_FAIL flush_setup_count got=%0d", dut.state_lsu_outstanding_load_count_V);

            @(negedge clk);
            dut.state_rob_entries_busy_U.ram[0] = 1'b0;
            dut.state_rob_entries_exception_U.ram[0] = 1'b1;
            dut.state_rob_entries_uop_uopc_U.ram[0] = 8'd1;
            dut.state_rob_entries_uop_inst_U.ram[0] = 32'h0000_0013;
            dut.state_rob_entries_uop_debug_pc_U.ram[0] = 64'h0000_0000_0000_1000;
            exception_seen = 0;
            redirect_seen = 0;
            flush_seen = 0;
            for (i = 0; i < 20000; i = i + 1) begin
                @(posedge clk); #1;
                if (dut.state_exception_commit_valid === 1'b1)
                    exception_seen = 1;
                if (dut.state_frontend_redirect_valid === 1'b1)
                    redirect_seen = 1;
                if (dut.state_global_flush === 1'b1)
                    flush_seen = 1;
                if (exception_seen && dut.state_lsu_outstanding_load_count_V === 0 && active_lq_count() == 0)
                    i = 20000;
            end
            if (!exception_seen || !redirect_seen || !flush_seen)
                $fatal(1, "G6_L3_R2_RTL_FAIL flush_exception_recovery_not_observed exception=%0d redirect=%0d flush=%0d",
                       exception_seen, redirect_seen, flush_seen);
            if (dut.state_lsu_outstanding_load_count_V !== 0 || active_lq_count() != 0 ||
                lq_valid(0) !== 1'b0 || lq_pending(0) !== 1'b0 ||
                lq_valid(1) !== 1'b0 || lq_pending(1) !== 1'b0)
                $fatal(1, "G6_L3_R2_RTL_FAIL flush_recovery_did_not_clear_lq count=%0d active=%0d lq0=%b/%b lq1=%b/%b",
                       dut.state_lsu_outstanding_load_count_V, active_lq_count(),
                       lq_valid(0), lq_pending(0), lq_valid(1), lq_pending(1));

            base_resp = response_count;
            send_stale_response_probe(32'h8000, DATA_BASE + 64'h8000, 8'd0, 8'd1,
                                      stale_accepted, wrong_completion, wrong_prf_writeback);
            wait_architectural_cycles(2);
            if (wrong_completion || wrong_prf_writeback ||
                dut.state_rob_entries_memory_completed_U.ram[0] === 1'b1 ||
                dut.state_rob_entries_memory_completed_U.ram[1] === 1'b1)
                $fatal(1, "G6_L3_R2_RTL_FAIL flush_stale_response_side_effect completion=%0d prf=%0d rob0=%b rob1=%b",
                       wrong_completion, wrong_prf_writeback,
                       dut.state_rob_entries_memory_completed_U.ram[0],
                       dut.state_rob_entries_memory_completed_U.ram[1]);
            stale_response_reject_count = stale_response_reject_count + 1;

            dut.state_rob_head = 5'd2;
            dut.state_rob_tail = 5'd3;
            dut.state_rob_maybe_full = 1'b0;
            dut.state_lsu_ldq_count_V = 1;
            dut.state_lsu_next_transaction_id = 32'h8002;
            seed_load(8'd2, 0, 32'h8002, 16'h0802, ADDR_BASE + 64'h8020, 5'd3);
            base_req = request_count;
            wait_requests(base_req + 1);
            expect_request(base_req, 32'h8002, 8'd2, ADDR_BASE + 64'h8020);
            expect_lq_pending(0, 32'h8002, 8'd2, 32'h8002, 16'h0802);
            send_response(32'h8002, DATA_BASE + 64'h8020);
            wait_load_done(8'd2, 0, DATA_BASE + 64'h8020);
            $display("G6_L3_R2_CASE_PASS case=flush_with_pending recovery=exception outstanding_before=2 exception_seen=%0d redirect_seen=%0d flush_seen=%0d stale_response_rejected=1 stale_accepted_at_port=%0d wrong_completion=0 wrong_prf_writeback=0 reexecute_load=1 reset_used=0 branch_mispredict_used=0 base_resp=%0d responses=%0d",
                     exception_seen, redirect_seen, flush_seen, stale_accepted,
                     base_resp, response_count);

            start_case("reset_with_pending");
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_count_V = 1;
            dut.state_lsu_next_transaction_id = 32'h8100;
            seed_load(8'd0, 0, 32'h8100, 16'h0810, ADDR_BASE + 64'h8100, 5'd1);
            wait_requests(request_count + 1);
            if (dut.state_lsu_outstanding_load_count_V !== 1 || active_lq_count() != 1)
                $fatal(1, "G6_L3_R2_RTL_FAIL reset_pending_setup count=%0d active=%0d",
                       dut.state_lsu_outstanding_load_count_V, active_lq_count());
            reset_core();
            if (dut.state_lsu_outstanding_load_count_V !== 0 || active_lq_count() != 0)
                $fatal(1, "G6_L3_R2_RTL_FAIL reset_pending_clear count=%0d active=%0d",
                       dut.state_lsu_outstanding_load_count_V, active_lq_count());
            $display("G6_L3_R2_CASE_PASS case=reset_with_pending reset_cleared_pending=1 exception_recovery_used=0 branch_mispredict_used=0");
        end
    endtask

    initial begin
        if (!(LQ_DEPTH == 4 || LQ_DEPTH == 8 || LQ_DEPTH == 16))
            $fatal(1, "G6_L3_R2_RTL_FAIL unsupported_lq_depth %0d", LQ_DEPTH);
        if (!(SQ_DEPTH == 4 || SQ_DEPTH == 8 || SQ_DEPTH == 16))
            $fatal(1, "G6_L3_R2_RTL_FAIL unsupported_sq_depth %0d", SQ_DEPTH);
        $display("G6_L3_R2_RTL_CONFIG_START config=%0d_%0d", LQ_DEPTH, SQ_DEPTH);
        run_saturation_reuse_stale();
        run_txid_wrap_duplicate_skip();
        run_request_backpressure();
        run_response_backpressure_and_consecutive();
        run_store_forwarding();
        run_flush_with_pending();
        if (case_failures == 0)
            $display("G6_L3_R2_RTL_CONFIG_%0d_%0d_PASS cases=%0d requests=%0d responses=%0d max_outstanding=%0d response_backpressure_blocked_cycles=%0d stale_response_reject_count=%0d",
                      LQ_DEPTH, SQ_DEPTH, case_count, request_count, response_count,
                      max_observed_outstanding, response_backpressure_blocked_cycles,
                      stale_response_reject_count);
        else
            $display("G6_L3_R2_RTL_CONFIG_%0d_%0d_FAIL cases=%0d failures=%0d requests=%0d responses=%0d max_outstanding=%0d response_backpressure_blocked_cycles=%0d stale_response_reject_count=%0d",
                      LQ_DEPTH, SQ_DEPTH, case_count, case_failures, request_count, response_count,
                      max_observed_outstanding, response_backpressure_blocked_cycles,
                      stale_response_reject_count);
        $finish;
    end
endmodule

`undef LQ_VALID_CASE
`undef LQ_PENDING_CASE
`undef LQ_TXID_CASE
`undef LQ_ROB_CASE
`undef LQ_ALLOC_CASE
`undef LQ_GEN_CASE
`undef SEED_LQ_CASE
`undef G6_L3_INTERNAL_DMEM_TDATA
`undef G6_L3_INTERNAL_DMEM_TVALID
`undef G6_L3_INTERNAL_DMEM_TREADY
`undef G6_L3_STQ_VALID0
`undef G6_L3_STQ_ADDR_VALID0
`undef G6_L3_STQ_DATA_VALID0
`undef G6_L3_STQ_ROB0
`undef G6_L3_STQ_MASK0
`undef G6_L3_STQ_ALLOC0
`undef G6_L3_STQ_GEN0
`undef G6_L3_STQ_ADDR0
`undef G6_L3_STQ_DATA0
