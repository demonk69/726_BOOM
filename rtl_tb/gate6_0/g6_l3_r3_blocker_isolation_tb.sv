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
`define G6_L3_STQ_ADDR0 dut.boom_core_top_stream_stream_stream_DmemRequest_0_stream_DmemResponse_0_stream_CommitEntry_0_bool_volatile_bool_volatile_bool_volatile_bool_volatile_unsigned_long_volatile_unsigned_long_volatile_state_U.ram[0]
`define G6_L3_STQ_DATA0 dut.boom_core_top_stream_stream_stream_DmemRequest_0_stream_DmemResponse_0_stream_CommitEntry_0_bool_volatile_bool_volatile_bool_volatile_bool_volatile_unsigned_long_volatile_unsigned_long_volatile_state_2_U.ram[0]
`elsif G6_L3_HAS_SQ8
`define G6_L3_STQ_ADDR0 dut.state_lsu_stq_address_U.ram[0]
`define G6_L3_STQ_DATA0 dut.state_lsu_stq_data_U.ram[0]
`else
`define G6_L3_STQ_ADDR0 dut.state_lsu_stq_address_0
`define G6_L3_STQ_DATA0 dut.state_lsu_stq_data_0
`endif

module g6_l3_r3_blocker_isolation_tb;
    localparam integer LQ_DEPTH = `G6_L3_LQ_DEPTH;
    localparam integer SQ_DEPTH = `G6_L3_SQ_DEPTH;
    localparam integer MAX_TRACKED = 128;
    localparam [31:0] TX_BASE = 32'h0000_9000;
    localparam [63:0] ADDR_BASE = 64'h0000_0000_9000_0000;
    localparam [63:0] DATA_BASE = 64'h9000_0000_0000_0000;

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
    integer max_observed_outstanding = 0;
    integer req_fd = 0;
    integer fwd_fd = 0;
    integer flush_fd = 0;
    integer result_failures = 0;
    reg [8*96-1:0] current_case = "init";

    reg [31:0] req_txid [0:MAX_TRACKED-1];
    reg [7:0] req_rob [0:MAX_TRACKED-1];
    reg [7:0] req_command [0:MAX_TRACKED-1];
    reg [7:0] req_size [0:MAX_TRACKED-1];
    reg [7:0] req_mask [0:MAX_TRACKED-1];
    reg [63:0] req_address [0:MAX_TRACKED-1];

    integer bp_tvalid_hold_verified = 0;
    integer bp_request_loss_count = 0;
    integer bp_request_duplication_count = 0;
    reg [8*128-1:0] bp_root_cause = "UNSET";
    integer fwd_seed_valid = 0;
    integer fwd_rtl_pass = 0;
    integer fwd_isolated_load_requests = 0;
    integer fwd_isolated_store_requests = 0;
    integer fwd_r2_load_requests = 0;
    integer fwd_r2_store_requests = 0;
    integer flush_recovery_path_valid = 0;
    integer flush_rtl_pass = 0;
    integer flush_stale_response_rejected = 0;

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
        #1;
        cycles = cycles + 1;
        if (cycles > 1000000) $fatal(1, "G6_L3_R3_RTL_FAIL timeout");
        if (rst_n) begin
            if (dut.state_lsu_outstanding_load_count_V > max_observed_outstanding)
                max_observed_outstanding = dut.state_lsu_outstanding_load_count_V;
            if (dmem_req_tvalid && dmem_req_tready) begin
                if (request_count >= MAX_TRACKED) $fatal(1, "G6_L3_R3_RTL_FAIL request_overflow");
                req_txid[request_count] = dmem_req_tdata[31:0];
                req_rob[request_count] = dmem_req_tdata[39:32];
                req_command[request_count] = dmem_req_tdata[47:40];
                req_size[request_count] = dmem_req_tdata[55:48];
                req_mask[request_count] = dmem_req_tdata[63:56];
                req_address[request_count] = dmem_req_tdata[191:128];
                $display("G6_L3_R3_TRACE config=%0d_%0d case=%0s event=request index=%0d txid=%h rob=%0d cmd=%0d outstanding=%0d address=%h",
                         LQ_DEPTH, SQ_DEPTH, current_case, request_count,
                         dmem_req_tdata[31:0], dmem_req_tdata[39:32], dmem_req_tdata[47:40],
                         dut.state_lsu_outstanding_load_count_V, dmem_req_tdata[191:128]);
                request_count = request_count + 1;
            end
            if (dmem_resp_tvalid && dmem_resp_tready) begin
                $display("G6_L3_R3_TRACE config=%0d_%0d case=%0s event=response index=%0d txid=%h outstanding=%0d",
                         LQ_DEPTH, SQ_DEPTH, current_case, response_count,
                         dmem_resp_tdata[31:0], dut.state_lsu_outstanding_load_count_V);
                response_count = response_count + 1;
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

    task automatic open_trace_files;
        begin
            req_fd = $fopen("request_backpressure_trace.csv", "w");
            fwd_fd = $fopen("store_forwarding_trace.csv", "w");
            flush_fd = $fopen("flush_recovery_trace.csv", "w");
            if (req_fd == 0 || fwd_fd == 0 || flush_fd == 0)
                $fatal(1, "G6_L3_R3_RTL_FAIL trace_open_failed");
            $fwrite(req_fd, "config,cycle,phase,top_valid,top_ready,top_handshake,top_txid,top_rob,top_cmd,top_addr,internal_valid,internal_ready,internal_handshake,internal_txid,internal_rob,internal_cmd,internal_addr,regslice_state,owner_sent,outstanding,lq0_valid,lq0_pending,lq0_txid,event\n");
            $fwrite(fwd_fd, "config,cycle,phase,top_valid,top_ready,handshake,txid,rob,cmd,address,store_valid,store_addr_valid,store_data_valid,store_busy,load_completed,load_data,outstanding,event\n");
            $fwrite(flush_fd, "config,cycle,phase,outstanding,active_lq,lq0_valid,lq0_pending,lq0_txid,lq1_valid,lq1_pending,lq1_txid,rob0_valid,rob1_valid,global_flush,exception_commit_valid,frontend_redirect_valid,dmem_resp_valid,dmem_resp_ready,event\n");
        end
    endtask

    task automatic trace_request(input [8*64-1:0] phase, input [8*96-1:0] ev_label);
        begin
            $fwrite(req_fd,
                    "%0d_%0d,%0d,%0s,%0b,%0b,%0b,%08h,%0d,%0d,%016h,%0b,%0b,%0b,%08h,%0d,%0d,%016h,%0d,%0b,%0d,%0b,%0b,%08h,%0s\n",
                    LQ_DEPTH, SQ_DEPTH, cycles, phase,
                    dmem_req_tvalid, dmem_req_tready, dmem_req_tvalid && dmem_req_tready,
                    dmem_req_tdata[31:0], dmem_req_tdata[39:32], dmem_req_tdata[47:40], dmem_req_tdata[191:128],
                    `G6_L3_INTERNAL_DMEM_TVALID,
                    `G6_L3_INTERNAL_DMEM_TREADY,
                    `G6_L3_INTERNAL_DMEM_TVALID && `G6_L3_INTERNAL_DMEM_TREADY,
                    `G6_L3_INTERNAL_DMEM_TDATA[31:0],
                    `G6_L3_INTERNAL_DMEM_TDATA[39:32],
                    `G6_L3_INTERNAL_DMEM_TDATA[47:40],
                    `G6_L3_INTERNAL_DMEM_TDATA[191:128],
                    dut.regslice_both_dmem_req_out_U.B_V_data_1_state,
                    dut.state_rob_entries_memory_request_sent_U.ram[0],
                    dut.state_lsu_outstanding_load_count_V,
                    lq_valid(0), lq_pending(0), lq_txid(0), ev_label);
        end
    endtask

    task automatic trace_forwarding(input [8*64-1:0] phase, input [8*96-1:0] ev_label);
        begin
            $fwrite(fwd_fd,
                    "%0d_%0d,%0d,%0s,%0b,%0b,%0b,%08h,%0d,%0d,%016h,%0b,%0b,%0b,%0b,%0b,%016h,%0d,%0s\n",
                    LQ_DEPTH, SQ_DEPTH, cycles, phase,
                    dmem_req_tvalid, dmem_req_tready, dmem_req_tvalid && dmem_req_tready,
                    dmem_req_tdata[31:0], dmem_req_tdata[39:32], dmem_req_tdata[47:40], dmem_req_tdata[191:128],
`ifdef G6_L3_HAS_SQ8
                    dut.state_lsu_stq_valid_U.ram[0], dut.state_lsu_stq_address_valid_U.ram[0],
                    dut.state_lsu_stq_data_valid_U.ram[0], dut.state_rob_entries_busy_U.ram[0],
`else
                    dut.state_lsu_stq_valid_0, dut.state_lsu_stq_address_valid_0,
                    dut.state_lsu_stq_data_valid_0, dut.state_rob_entries_busy_U.ram[0],
`endif
                    dut.state_rob_entries_memory_completed_U.ram[1],
                    dut.state_rob_entries_memory_data_U.ram[1],
                    dut.state_lsu_outstanding_load_count_V, ev_label);
        end
    endtask

    task automatic trace_flush(input [8*64-1:0] phase, input [8*96-1:0] ev_label);
        begin
            $fwrite(flush_fd,
                    "%0d_%0d,%0d,%0s,%0d,%0d,%0b,%0b,%08h,%0b,%0b,%08h,%0b,%0b,%0b,%0b,%0b,%0b,%0b,%0s\n",
                    LQ_DEPTH, SQ_DEPTH, cycles, phase,
                    dut.state_lsu_outstanding_load_count_V, active_lq_count(),
                    lq_valid(0), lq_pending(0), lq_txid(0), lq_valid(1), lq_pending(1), lq_txid(1),
                    dut.state_rob_entries_valid_U.ram[0], dut.state_rob_entries_valid_U.ram[1],
                    dut.state_global_flush, dut.state_exception_commit_valid,
                    dut.state_frontend_redirect_valid, dmem_resp_tvalid, dmem_resp_tready, ev_label);
        end
    endtask

    task automatic reset_core;
        integer guard;
        begin
            rst_n = 1'b0;
            dmem_req_tready = 1'b1;
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
            repeat (5) @(negedge clk);
            rst_n = 1'b1;
            guard = 0;
            while (dut.reset_ctrl_completed !== 1'b1 && guard < 40000) begin
                @(posedge clk); #1;
                guard = guard + 1;
            end
            if (guard >= 40000)
                $fatal(1, "G6_L3_R3_RTL_FAIL reset_ctrl_timeout fsm=%h state34=%b state35=%b block=%b deadlock=%b",
                       dut.ap_CS_fsm, dut.ap_CS_fsm_state34, dut.ap_CS_fsm_state35,
                       ap_local_block, ap_local_deadlock);
            guard = 0;
            while (dut.ap_CS_fsm_state34 !== 1'b1 && guard < 40000) begin
                @(posedge clk); #1;
                guard = guard + 1;
            end
            if (guard >= 40000)
                $fatal(1, "G6_L3_R3_RTL_FAIL reset_idle_timeout fsm=%h reset_done=%b state34=%b state35=%b block=%b deadlock=%b",
                       dut.ap_CS_fsm, dut.reset_ctrl_completed, dut.ap_CS_fsm_state34,
                       dut.ap_CS_fsm_state35, ap_local_block, ap_local_deadlock);
            @(negedge clk);
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
                default: $fatal(1, "G6_L3_R3_RTL_FAIL bad_lq_slot %0d depth=%0d", slot, LQ_DEPTH);
            endcase
        end
    endtask

    task automatic seed_rob_load_only(input [7:0] rob_idx, input integer lq_idx,
                                      input [31:0] allocation, input [15:0] generation,
                                      input [63:0] address, input [4:0] tail_after,
                                      input [7:0] branch_mask);
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
            dut.state_rob_entries_uop_iw_p1_poisoned_1_U.ram[rob_idx] = branch_mask;
        end
    endtask

    task automatic seed_load(input [7:0] rob_idx, input integer lq_idx,
                             input [31:0] allocation, input [15:0] generation,
                             input [63:0] address, input [4:0] tail_after,
                             input [7:0] branch_mask);
        begin
            seed_rob_load_only(rob_idx, lq_idx, allocation, generation, address, tail_after, branch_mask);
            seed_lq(lq_idx, rob_idx, allocation, generation, branch_mask);
        end
    endtask

    task automatic seed_store0(input [7:0] rob_idx, input [31:0] allocation,
                               input [15:0] generation, input [63:0] address,
                               input [63:0] data, input [0:0] busy);
        begin
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
            dut.state_rob_entries_memory_mask_U.ram[rob_idx] = 8'hff;
            dut.state_rob_entries_memory_size_U.ram[rob_idx] = 8'd3;
            dut.state_rob_entries_uop_iq_type_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_uop_imm_packed_5_U.ram[rob_idx] = 8'd0;
            dut.state_rob_entries_uop_imm_packed_U.ram[rob_idx] = generation;
            dut.state_rob_entries_uop_imm_packed_2_U.ram[rob_idx] = rob_idx[4:0];
            dut.state_rob_entries_uop_imm_packed_4_U.ram[rob_idx] = allocation;
            dut.state_lsu_stq_count_V = 5'd1;
`ifdef G6_L3_HAS_SQ8
            dut.state_lsu_stq_valid_U.ram[0] = 1'b1;
            dut.state_lsu_stq_address_valid_U.ram[0] = 1'b1;
            dut.state_lsu_stq_data_valid_U.ram[0] = 1'b1;
            dut.state_lsu_stq_rob_idx_U.ram[0] = rob_idx;
            dut.state_lsu_stq_mask_U.ram[0] = 8'hff;
            dut.state_lsu_stq_rob_allocation_id_U.ram[0] = allocation;
            dut.state_lsu_stq_generation_U.ram[0] = generation;
            `G6_L3_STQ_ADDR0 = address;
            `G6_L3_STQ_DATA0 = data;
`else
            dut.state_lsu_stq_valid_0 = 1'b1;
            dut.state_lsu_stq_address_valid_0 = 1'b1;
            dut.state_lsu_stq_data_valid_0 = 1'b1;
            dut.state_lsu_stq_rob_idx_0 = rob_idx;
            dut.state_lsu_stq_mask_0 = 8'hff;
            dut.state_lsu_stq_rob_allocation_id_0 = allocation;
            dut.state_lsu_stq_generation_0 = generation;
            dut.state_lsu_stq_address_0 = address;
            dut.state_lsu_stq_data_0 = data;
`endif
        end
    endtask

    task automatic wait_architectural_cycles(input integer count);
        integer n;
        integer guard;
        begin
            for (n = 0; n < count; n = n + 1) begin
                guard = 0;
                while (dut.ap_CS_fsm_state34 === 1'b1 && guard < 80000) begin
                    @(posedge clk); #1;
                    guard = guard + 1;
                end
                if (guard >= 80000) $fatal(1, "G6_L3_R3_RTL_FAIL arch_leave_timeout case=%0s", current_case);
                guard = 0;
                while (dut.ap_CS_fsm_state34 !== 1'b1 && guard < 80000) begin
                    @(posedge clk); #1;
                    guard = guard + 1;
                end
                if (guard >= 80000) $fatal(1, "G6_L3_R3_RTL_FAIL arch_return_timeout case=%0s", current_case);
            end
            @(negedge clk);
        end
    endtask

    task automatic wait_requests(input integer target);
        integer guard;
        begin
            guard = 0;
            while (request_count < target && guard < 80000) begin
                @(posedge clk); #1;
                guard = guard + 1;
            end
            if (guard >= 80000)
                $fatal(1, "G6_L3_R3_RTL_FAIL request_timeout target=%0d got=%0d case=%0s fsm=%h block=%b deadlock=%b",
                       target, request_count, current_case, dut.ap_CS_fsm, ap_local_block, ap_local_deadlock);
            @(negedge clk);
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
                @(posedge clk); #1;
                guard = guard + 1;
            end
            if (guard >= 80000) $fatal(1, "G6_L3_R3_RTL_FAIL response_ready_timeout txid=%h case=%0s", txid, current_case);
            @(posedge clk); #1;
            @(negedge clk);
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
        end
    endtask

    task automatic wait_load_done(input [7:0] rob_idx, input integer lq_idx,
                                  input [63:0] expected_data, output integer done_ok);
        integer guard;
        begin
            guard = 0;
            done_ok = 0;
            while ((dut.state_rob_entries_memory_completed_U.ram[rob_idx] !== 1'b1 ||
                    lq_valid(lq_idx) !== 1'b0 || lq_pending(lq_idx) !== 1'b0) &&
                   guard < 80000) begin
                @(posedge clk); #1;
                guard = guard + 1;
            end
            if (guard < 80000 && dut.state_rob_entries_busy_U.ram[rob_idx] === 1'b0 &&
                dut.state_rob_entries_memory_data_U.ram[rob_idx] === expected_data)
                done_ok = 1;
            @(negedge clk);
        end
    endtask

    task automatic run_request_backpressure;
        integer base_req;
        integer i;
        integer top_valid_seen;
        integer top_drop;
        integer payload_change;
        integer internal_handshakes;
        integer handshakes_while_blocked;
        integer release_base;
        integer done_ok;
        reg [319:0] held_req;
        begin
            current_case = "request_backpressure";
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_count_V = 1;
            dut.state_lsu_next_transaction_id = 32'h5000;
            seed_load(8'd0, 0, 32'h5000, 16'h0500, ADDR_BASE + 64'h5000, 5'd1, 8'd0);
            base_req = request_count;
            dmem_req_tready = 1'b0;
            top_valid_seen = 0;
            top_drop = 0;
            payload_change = 0;
            internal_handshakes = 0;
            handshakes_while_blocked = 0;
            held_req = 320'd0;
            for (i = 0; i < 800; i = i + 1) begin
                @(posedge clk); #1;
                if (`G6_L3_INTERNAL_DMEM_TVALID &&
                    `G6_L3_INTERNAL_DMEM_TREADY)
                    internal_handshakes = internal_handshakes + 1;
                if (dmem_req_tvalid && dmem_req_tready)
                    handshakes_while_blocked = handshakes_while_blocked + 1;
                if (!top_valid_seen && dmem_req_tvalid) begin
                    top_valid_seen = 1;
                    held_req = dmem_req_tdata;
                    trace_request("hold", "top_valid_first_seen");
                end else if (top_valid_seen && dmem_req_tready === 1'b0) begin
                    if (dmem_req_tvalid !== 1'b1)
                        top_drop = top_drop + 1;
                    else if (dmem_req_tdata !== held_req)
                        payload_change = payload_change + 1;
                end
                if ((i % 10) == 0 || dmem_req_tvalid || `G6_L3_INTERNAL_DMEM_TVALID ||
                    dut.state_rob_entries_memory_request_sent_U.ram[0])
                    trace_request("hold", "sample");
            end

            if (handshakes_while_blocked != 0) begin
                bp_root_cause = "RTL_PROTOCOL_VIOLATION_HANDSHAKE_WHILE_TREADY_LOW";
            end else if (top_valid_seen && (top_drop != 0 || payload_change != 0)) begin
                bp_root_cause = "RTL_PROTOCOL_VIOLATION_TVALID_OR_PAYLOAD_NOT_HELD";
            end else if (top_valid_seen && dut.state_rob_entries_memory_request_sent_U.ram[0] === 1'b1) begin
                bp_root_cause = "TESTBENCH_ASSERTION_TOO_EARLY_OWNER_RESERVED_BEFORE_EXTERNAL_HANDSHAKE";
                bp_tvalid_hold_verified = 1;
            end else begin
                bp_root_cause = "UNRESOLVED_NO_TOP_VALID_AFTER_OWNER_RESERVED";
            end

            release_base = request_count;
            trace_request("release", "release_ready");
            dmem_req_tready = 1'b1;
            wait_requests(release_base + 1);
            trace_request("release", "released_handshake_observed");
            if (request_count > release_base + 1)
                bp_request_duplication_count = request_count - (release_base + 1);
            if (request_count < base_req + 1)
                bp_request_loss_count = (base_req + 1) - request_count;
            if (req_txid[release_base] !== 32'h5000 || req_rob[release_base] !== 8'd0 ||
                req_command[release_base] !== 8'd0 || req_address[release_base] !== ADDR_BASE + 64'h5000)
                bp_request_loss_count = bp_request_loss_count + 1;
            send_response(32'h5000, DATA_BASE + 64'h5000);
            wait_load_done(8'd0, 0, DATA_BASE + 64'h5000, done_ok);
            if (done_ok != 1)
                bp_request_loss_count = bp_request_loss_count + 1;
            if (bp_tvalid_hold_verified == 0 || bp_request_loss_count != 0 || bp_request_duplication_count != 0)
                result_failures = result_failures + 1;
            $display("G6_L3_R3_RESULT config=%0d_%0d key=BACKPRESSURE_ROOT_CAUSE value=%0s", LQ_DEPTH, SQ_DEPTH, bp_root_cause);
            $display("G6_L3_R3_RESULT config=%0d_%0d key=TVALID_HOLD_VERIFIED value=%0d", LQ_DEPTH, SQ_DEPTH, bp_tvalid_hold_verified);
            $display("G6_L3_R3_RESULT config=%0d_%0d key=REQUEST_LOSS_COUNT value=%0d", LQ_DEPTH, SQ_DEPTH, bp_request_loss_count);
            $display("G6_L3_R3_RESULT config=%0d_%0d key=REQUEST_DUPLICATION_COUNT value=%0d", LQ_DEPTH, SQ_DEPTH, bp_request_duplication_count);
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

    task automatic run_store_forwarding_variant(input [8*64-1:0] phase,
                                                input [0:0] store_busy,
                                                output integer load_reqs,
                                                output integer store_reqs,
                                                output integer load_done);
        integer base_req;
        integer i;
        begin
            reset_core();
            current_case = phase;
            dut.state_rob_head = 5'd0;
            dut.state_rob_tail = 5'd2;
            dut.state_rob_maybe_full = 1'b0;
            seed_store0(8'd0, 32'h7000, 16'h0700, ADDR_BASE + 64'h7000,
                        64'h1234_5678_9abc_def0, store_busy);
            dut.state_lsu_ldq_count_V = 1;
            seed_load(8'd1, 0, 32'h7001, 16'h0701, ADDR_BASE + 64'h7000, 5'd2, 8'd0);
            base_req = request_count;
            for (i = 0; i < 300; i = i + 1) begin
                @(posedge clk); #1;
                if ((i % 10) == 0 || dmem_req_tvalid || dut.state_rob_entries_memory_completed_U.ram[1])
                    trace_forwarding(phase, "sample");
                if (dut.state_rob_entries_memory_completed_U.ram[1] === 1'b1 &&
                    dut.state_rob_entries_memory_data_U.ram[1] === 64'h1234_5678_9abc_def0)
                    i = 300;
            end
            wait_architectural_cycles(2);
            trace_forwarding(phase, "post_wait");
            count_requests(base_req, request_count, load_reqs, store_reqs);
            load_done = (dut.state_rob_entries_memory_completed_U.ram[1] === 1'b1 &&
                         dut.state_rob_entries_memory_data_U.ram[1] === 64'h1234_5678_9abc_def0 &&
                         lq_valid(0) === 1'b0 && lq_pending(0) === 1'b0);
            trace_forwarding(phase, "final");
        end
    endtask

    task automatic run_store_forwarding;
        integer done_isolated;
        integer done_r2;
        begin
            current_case = "store_forwarding";
            run_store_forwarding_variant("forwarding_isolated_busy_store", 1'b1,
                                         fwd_isolated_load_requests,
                                         fwd_isolated_store_requests,
                                         done_isolated);
            run_store_forwarding_variant("forwarding_r2_seed_committable_store", 1'b0,
                                         fwd_r2_load_requests,
                                         fwd_r2_store_requests,
                                         done_r2);
            fwd_seed_valid = (done_isolated == 1 && fwd_isolated_load_requests == 0 &&
                              fwd_isolated_store_requests == 0);
            fwd_rtl_pass = (fwd_seed_valid == 1 && done_r2 == 1 && fwd_r2_load_requests == 0);
            if (!fwd_rtl_pass)
                result_failures = result_failures + 1;
            $display("G6_L3_R3_RESULT config=%0d_%0d key=FORWARDING_SEED_VALID value=%0d", LQ_DEPTH, SQ_DEPTH, fwd_seed_valid);
            $display("G6_L3_R3_RESULT config=%0d_%0d key=FORWARDING_RTL_STATUS value=%0s", LQ_DEPTH, SQ_DEPTH, fwd_rtl_pass ? "PASS" : "FAIL");
            $display("G6_L3_R3_RESULT config=%0d_%0d key=FORWARDING_EXTERNAL_REQUESTS value=isolated_loads:%0d;isolated_stores:%0d;r2_loads:%0d;r2_stores:%0d",
                     LQ_DEPTH, SQ_DEPTH, fwd_isolated_load_requests, fwd_isolated_store_requests,
                     fwd_r2_load_requests, fwd_r2_store_requests);
        end
    endtask

    task automatic send_stale_response_probe(input [31:0] txid, input [63:0] data,
                                             output integer accepted);
        integer i;
        begin
            accepted = 0;
            @(negedge clk);
            dmem_resp_tdata = 384'd0;
            dmem_resp_tdata[31:0] = txid;
            dmem_resp_tdata[127:64] = data;
            dmem_resp_tdata[191:128] = data;
            dmem_resp_tvalid = 1'b1;
            for (i = 0; i < 40; i = i + 1) begin
                @(posedge clk); #1;
                trace_flush("stale_response", "sample");
                if (dmem_resp_tready === 1'b1)
                    accepted = 1;
            end
            @(negedge clk);
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
        end
    endtask

    task automatic run_flush_recovery;
        integer base_req;
        integer i;
        integer exception_seen;
        integer redirect_seen;
        integer flush_seen;
        integer stale_accepted;
        begin
            current_case = "flush_recovery_exception";
            reset_core();
            dut.state_rob_head = 5'd0;
            dut.state_rob_tail = 5'd2;
            dut.state_rob_maybe_full = 1'b0;
            dut.state_lsu_ldq_count_V = 2;
            dut.state_lsu_next_transaction_id = 32'h8000;
            seed_load(8'd0, 0, 32'h8000, 16'h0800, ADDR_BASE + 64'h8000, 5'd2, 8'd0);
            seed_load(8'd1, 1, 32'h8001, 16'h0801, ADDR_BASE + 64'h8008, 5'd2, 8'd0);
            base_req = request_count;
            wait_requests(base_req + 2);
            trace_flush("setup", "outstanding_requests_issued");
            if (dut.state_lsu_outstanding_load_count_V !== 2 || active_lq_count() != 2) begin
                flush_rtl_pass = 0;
                result_failures = result_failures + 1;
                $display("G6_L3_R3_RESULT config=%0d_%0d key=FLUSH_RECOVERY_PATH_VALID value=0", LQ_DEPTH, SQ_DEPTH);
                $display("G6_L3_R3_RESULT config=%0d_%0d key=FLUSH_RTL_STATUS value=FAIL_SETUP", LQ_DEPTH, SQ_DEPTH);
                $display("G6_L3_R3_RESULT config=%0d_%0d key=STALE_RESPONSE_REJECTED value=0", LQ_DEPTH, SQ_DEPTH);
            end else begin
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
                    if ((i % 50) == 0 || dut.state_exception_commit_valid || dut.state_global_flush)
                        trace_flush("exception_recovery", "sample");
                    if (exception_seen && dut.state_lsu_outstanding_load_count_V === 0 && active_lq_count() == 0)
                        i = 20000;
                end
                flush_recovery_path_valid = exception_seen && redirect_seen && flush_seen;
                trace_flush("exception_recovery", "post_recovery");
                send_stale_response_probe(32'h8000, DATA_BASE + 64'h8000, stale_accepted);
                wait_architectural_cycles(2);
                trace_flush("stale_response", stale_accepted ? "accepted_at_port" : "not_accepted_at_port");
                flush_stale_response_rejected = (dut.state_lsu_outstanding_load_count_V === 0 && active_lq_count() == 0 &&
                                                 dut.state_rob_entries_memory_completed_U.ram[0] !== 1'b1 &&
                                                 dut.state_rob_entries_memory_completed_U.ram[1] !== 1'b1);
                flush_rtl_pass = (flush_recovery_path_valid && flush_stale_response_rejected &&
                                  dut.state_lsu_outstanding_load_count_V === 0 && active_lq_count() == 0);
                if (!flush_rtl_pass)
                    result_failures = result_failures + 1;
                $display("G6_L3_R3_RESULT config=%0d_%0d key=FLUSH_RECOVERY_PATH_VALID value=%0d", LQ_DEPTH, SQ_DEPTH, flush_recovery_path_valid);
                $display("G6_L3_R3_RESULT config=%0d_%0d key=FLUSH_RTL_STATUS value=%0s", LQ_DEPTH, SQ_DEPTH, flush_rtl_pass ? "PASS" : "FAIL");
                $display("G6_L3_R3_RESULT config=%0d_%0d key=STALE_RESPONSE_REJECTED value=%0d", LQ_DEPTH, SQ_DEPTH, flush_stale_response_rejected);
            end
        end
    endtask

    initial begin
        if (!(LQ_DEPTH == 4 || LQ_DEPTH == 8 || LQ_DEPTH == 16))
            $fatal(1, "G6_L3_R3_RTL_FAIL unsupported_lq_depth %0d", LQ_DEPTH);
        if (!(SQ_DEPTH == 4 || SQ_DEPTH == 8 || SQ_DEPTH == 16))
            $fatal(1, "G6_L3_R3_RTL_FAIL unsupported_sq_depth %0d", SQ_DEPTH);
        open_trace_files();
        $display("G6_L3_R3_RTL_CONFIG_START config=%0d_%0d", LQ_DEPTH, SQ_DEPTH);
        run_request_backpressure();
        run_store_forwarding();
        run_flush_recovery();
        if (result_failures == 0)
            $display("G6_L3_R3_RTL_CONFIG_%0d_%0d_PASS failures=%0d requests=%0d responses=%0d max_outstanding=%0d",
                     LQ_DEPTH, SQ_DEPTH, result_failures, request_count, response_count, max_observed_outstanding);
        else
            $display("G6_L3_R3_RTL_CONFIG_%0d_%0d_FAIL failures=%0d requests=%0d responses=%0d max_outstanding=%0d",
                     LQ_DEPTH, SQ_DEPTH, result_failures, request_count, response_count, max_observed_outstanding);
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
