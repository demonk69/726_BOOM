`timescale 1ns/1ps

module g6_l3_multi_outstanding_tb;
    reg clk = 0;
    reg rst_n = 0;
    wire [191:0] imem_req_tdata;
    wire imem_req_tvalid;
    wire [319:0] dmem_req_tdata;
    wire dmem_req_tvalid;
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
    reg [31:0] req_txid [0:15];
    reg [7:0] req_rob [0:15];
    reg [7:0] req_command [0:15];
    reg [7:0] req_size [0:15];
    reg [7:0] req_mask [0:15];
    reg [63:0] req_address [0:15];

    localparam [31:0] TX0 = 32'h0000_0010;
    localparam [31:0] TX1 = 32'h0000_0011;
    localparam [31:0] TX2 = 32'h0000_0012;
    localparam [31:0] TX3 = 32'h0000_0013;
    localparam [31:0] TX4 = 32'h0000_0014;
    localparam [63:0] ADDR0 = 64'h0000_0000_8000_1000;
    localparam [63:0] ADDR1 = 64'h0000_0000_8000_1008;
    localparam [63:0] ADDR2 = 64'h0000_0000_8000_1010;
    localparam [63:0] ADDR3 = 64'h0000_0000_8000_1018;
    localparam [63:0] ADDR4 = 64'h0000_0000_8000_1020;
    localparam [63:0] DATA0 = 64'h1111_2222_3333_4444;
    localparam [63:0] DATA1 = 64'h5555_6666_7777_8888;
    localparam [63:0] DATA2 = 64'h9999_aaaa_bbbb_cccc;
    localparam [63:0] DATA3 = 64'hdddd_eeee_ffff_0001;
    localparam [63:0] DATA4 = 64'h1234_5678_9abc_def0;

    assign dmem_resp_tdata_unused = dmem_resp_tdata;

    boom_core_top dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk), .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata), .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(1'b0), .imem_resp_in_TDATA(256'd0),
        .imem_resp_in_TVALID(1'b0), .imem_resp_in_TREADY(),
        .dmem_req_out_TDATA(dmem_req_tdata), .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(1'b1), .dmem_resp_in_TDATA(dmem_resp_tdata_unused),
        .dmem_resp_in_TVALID(dmem_resp_tvalid), .dmem_resp_in_TREADY(dmem_resp_tready),
        .commit_trace_out_TDATA(trace_tdata), .commit_trace_out_TVALID(trace_tvalid),
        .commit_trace_out_TREADY(1'b1), .io_success(io_success),
        .io_halted(io_halted), .io_trap(io_trap),
        .io_cycle_valid(io_cycle_valid), .io_cycle(io_cycle), .io_instret(io_instret));

    always #5 clk = ~clk;

    always @(posedge clk) begin
        cycles <= cycles + 1;
        if (cycles > 100000) $fatal(1, "G6_L3_RTL_FAIL timeout");
        if (!rst_n) begin
            request_count <= 0;
            response_count <= 0;
        end else begin
            if (dmem_req_tvalid) begin
                req_txid[request_count] <= dmem_req_tdata[31:0];
                req_rob[request_count] <= dmem_req_tdata[39:32];
                req_command[request_count] <= dmem_req_tdata[47:40];
                req_size[request_count] <= dmem_req_tdata[55:48];
                req_mask[request_count] <= dmem_req_tdata[63:56];
                req_address[request_count] <= dmem_req_tdata[191:128];
                request_count <= request_count + 1;
            end
            if (dmem_resp_tvalid && dmem_resp_tready) response_count <= response_count + 1;
        end
    end

    function [0:0] lq_valid(input integer slot);
        begin
            case (slot)
                0: lq_valid = dut.state_lsu_ldq_valid_0;
                1: lq_valid = dut.state_lsu_ldq_valid_1;
                2: lq_valid = dut.state_lsu_ldq_valid_2;
                3: lq_valid = dut.state_lsu_ldq_valid_3;
                default: lq_valid = 1'bx;
            endcase
        end
    endfunction

    function [0:0] lq_pending(input integer slot);
        begin
            case (slot)
                0: lq_pending = dut.state_lsu_ldq_response_pending_0;
                1: lq_pending = dut.state_lsu_ldq_response_pending_1;
                2: lq_pending = dut.state_lsu_ldq_response_pending_2;
                3: lq_pending = dut.state_lsu_ldq_response_pending_3;
                default: lq_pending = 1'bx;
            endcase
        end
    endfunction

    function [31:0] lq_txid(input integer slot);
        begin
            case (slot)
                0: lq_txid = dut.state_lsu_ldq_transaction_id_0;
                1: lq_txid = dut.state_lsu_ldq_transaction_id_1;
                2: lq_txid = dut.state_lsu_ldq_transaction_id_2;
                3: lq_txid = dut.state_lsu_ldq_transaction_id_3;
                default: lq_txid = 32'hxxxx_xxxx;
            endcase
        end
    endfunction

    task automatic wait_reset_complete;
        integer guard;
        begin
            guard = 0;
            while (dut.reset_ctrl_completed !== 1'b1 && guard < 20000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 20000)
                $fatal(1, "G6_L3_RTL_FAIL reset_ctrl_timeout fsm=%h state34=%b state35=%b block=%b deadlock=%b",
                       dut.ap_CS_fsm, dut.ap_CS_fsm_state34, dut.ap_CS_fsm_state35,
                       ap_local_block, ap_local_deadlock);
            guard = 0;
            while (dut.ap_CS_fsm_state34 !== 1'b1 && guard < 20000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 20000)
                $fatal(1, "G6_L3_RTL_FAIL reset_idle_timeout fsm=%h reset_done=%b state34=%b state35=%b block=%b deadlock=%b",
                       dut.ap_CS_fsm, dut.reset_ctrl_completed, dut.ap_CS_fsm_state34,
                       dut.ap_CS_fsm_state35, ap_local_block, ap_local_deadlock);
            @(negedge clk);
        end
    endtask

    task automatic reset_core;
        begin
            rst_n = 1'b0;
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
            repeat (5) @(negedge clk);
            rst_n = 1'b1;
            wait_reset_complete();
        end
    endtask

    task automatic seed_lq(input integer slot, input [7:0] rob_idx,
                           input [31:0] allocation, input [15:0] generation);
        begin
            case (slot)
                0: begin
                    dut.state_lsu_ldq_valid_0 = 1'b1;
                    dut.state_lsu_ldq_response_pending_0 = 1'b0;
                    dut.state_lsu_ldq_rob_idx_0 = rob_idx;
                    dut.state_lsu_ldq_branch_mask_0 = 8'd0;
                    dut.state_lsu_ldq_transaction_id_0 = 32'd0;
                    dut.state_lsu_ldq_rob_allocation_id_0 = allocation;
                    dut.state_lsu_ldq_generation_0 = generation;
                end
                1: begin
                    dut.state_lsu_ldq_valid_1 = 1'b1;
                    dut.state_lsu_ldq_response_pending_1 = 1'b0;
                    dut.state_lsu_ldq_rob_idx_1 = rob_idx;
                    dut.state_lsu_ldq_branch_mask_1 = 8'd0;
                    dut.state_lsu_ldq_transaction_id_1 = 32'd0;
                    dut.state_lsu_ldq_rob_allocation_id_1 = allocation;
                    dut.state_lsu_ldq_generation_1 = generation;
                end
                2: begin
                    dut.state_lsu_ldq_valid_2 = 1'b1;
                    dut.state_lsu_ldq_response_pending_2 = 1'b0;
                    dut.state_lsu_ldq_rob_idx_2 = rob_idx;
                    dut.state_lsu_ldq_branch_mask_2 = 8'd0;
                    dut.state_lsu_ldq_transaction_id_2 = 32'd0;
                    dut.state_lsu_ldq_rob_allocation_id_2 = allocation;
                    dut.state_lsu_ldq_generation_2 = generation;
                end
                3: begin
                    dut.state_lsu_ldq_valid_3 = 1'b1;
                    dut.state_lsu_ldq_response_pending_3 = 1'b0;
                    dut.state_lsu_ldq_rob_idx_3 = rob_idx;
                    dut.state_lsu_ldq_branch_mask_3 = 8'd0;
                    dut.state_lsu_ldq_transaction_id_3 = 32'd0;
                    dut.state_lsu_ldq_rob_allocation_id_3 = allocation;
                    dut.state_lsu_ldq_generation_3 = generation;
                end
                default: $fatal(1, "G6_L3_RTL_FAIL bad_lq_slot %0d", slot);
            endcase
        end
    endtask

    task automatic seed_load(input [7:0] rob_idx, input integer lq_idx,
                             input [31:0] allocation, input [15:0] generation,
                             input [63:0] address, input [4:0] tail_after);
        begin
            dut.state_rob_tail = tail_after;
            dut.state_rob_maybe_full = 1'b0;
            dut.state_rob_entries_valid_U.ram[rob_idx] = 1'b1;
            dut.state_rob_entries_busy_U.ram[rob_idx] = 1'b1;
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
            dut.state_rob_entries_uop_imm_packed_1_U.ram[rob_idx] = lq_idx[7:0];
            dut.state_rob_entries_uop_imm_packed_2_U.ram[rob_idx] = rob_idx[4:0];
            dut.state_rob_entries_uop_imm_packed_3_U.ram[rob_idx] = generation;
            dut.state_rob_entries_uop_imm_packed_4_U.ram[rob_idx] = allocation;
            dut.state_rob_entries_uop_iw_p1_poisoned_1_U.ram[rob_idx] = 8'd0;
            seed_lq(lq_idx, rob_idx, allocation, generation);
        end
    endtask

    task automatic seed_initial_four;
        begin
            dut.state_rob_head = 5'd0;
            dut.state_lsu_ldq_tail_V = 2'd0;
            dut.state_lsu_ldq_count_V = 3'd4;
            dut.state_lsu_outstanding_load_count_V = 3'd0;
            dut.state_lsu_next_transaction_id = TX0;
            dut.state_completion_load_response_valid = 1'b0;
            seed_load(8'd0, 0, 32'h0000_0100, 16'h0020, ADDR0, 5'd4);
            seed_load(8'd1, 1, 32'h0000_0101, 16'h0021, ADDR1, 5'd4);
            seed_load(8'd2, 2, 32'h0000_0102, 16'h0022, ADDR2, 5'd4);
            seed_load(8'd3, 3, 32'h0000_0103, 16'h0023, ADDR3, 5'd4);
        end
    endtask

    task automatic wait_requests(input integer target);
        begin
            while (request_count < target) @(posedge clk);
            @(negedge clk);
        end
    endtask

    task automatic expect_request(input integer index, input [31:0] txid,
                                  input [7:0] rob_idx, input [63:0] address);
        begin
            if (req_txid[index] !== txid || req_rob[index] !== rob_idx ||
                req_command[index] !== 8'd0 || req_size[index] !== 8'd3 ||
                req_mask[index] !== 8'hff || req_address[index] !== address) begin
                $display("G6_L3_RTL_DIAG request[%0d] txid=%h rob=%h cmd=%h size=%h mask=%h address=%h",
                         index, req_txid[index], req_rob[index], req_command[index],
                         req_size[index], req_mask[index], req_address[index]);
                $fatal(1, "G6_L3_RTL_FAIL request_mismatch index=%0d", index);
            end
        end
    endtask

    task automatic expect_lq_pending(input integer slot, input [31:0] txid);
        begin
            if (lq_valid(slot) !== 1'b1 || lq_pending(slot) !== 1'b1 ||
                lq_txid(slot) !== txid)
                $fatal(1, "G6_L3_RTL_FAIL lq_pending slot=%0d txid=%h valid=%b pending=%b got_txid=%h",
                       slot, txid, lq_valid(slot), lq_pending(slot), lq_txid(slot));
        end
    endtask

    task automatic expect_lq_reclaimed(input integer slot);
        begin
            if (lq_valid(slot) !== 1'b0 || lq_pending(slot) !== 1'b0)
                $fatal(1, "G6_L3_RTL_FAIL lq_not_reclaimed slot=%0d valid=%b pending=%b txid=%h",
                       slot, lq_valid(slot), lq_pending(slot), lq_txid(slot));
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
            while (dmem_resp_tready !== 1'b1 && guard < 10000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 10000) $fatal(1, "G6_L3_RTL_FAIL response_ready_timeout txid=%h", txid);
            @(posedge clk);
            @(negedge clk);
            dmem_resp_tvalid = 1'b0;
            dmem_resp_tdata = 384'd0;
        end
    endtask

    task automatic wait_load_done(input [7:0] rob_idx, input integer lq_idx,
                                  input [63:0] expected_data);
        integer guard;
        begin
            guard = 0;
            while ((dut.state_rob_entries_memory_completed_U.ram[rob_idx] !== 1'b1 ||
                    lq_valid(lq_idx) !== 1'b0 || lq_pending(lq_idx) !== 1'b0) &&
                   guard < 10000) begin
                @(posedge clk);
                guard = guard + 1;
            end
            if (guard >= 10000)
                $fatal(1, "G6_L3_RTL_FAIL completion_timeout rob=%0d lq=%0d completed=%b valid=%b pending=%b txid=%h",
                       rob_idx, lq_idx, dut.state_rob_entries_memory_completed_U.ram[rob_idx],
                       lq_valid(lq_idx), lq_pending(lq_idx), lq_txid(lq_idx));
            @(negedge clk);
            if (dut.state_rob_entries_busy_U.ram[rob_idx] !== 1'b0)
                $fatal(1, "G6_L3_RTL_FAIL rob_busy_after_completion rob=%0d", rob_idx);
            if (dut.state_rob_entries_memory_data_U.ram[rob_idx] !== expected_data)
                $fatal(1, "G6_L3_RTL_FAIL load_value rob=%0d got=%h expected=%h",
                       rob_idx, dut.state_rob_entries_memory_data_U.ram[rob_idx], expected_data);
            expect_lq_reclaimed(lq_idx);
        end
    endtask

    task automatic wait_architectural_cycles(input integer count);
        integer n;
        begin
            for (n = 0; n < count; n = n + 1) begin
                while (dut.ap_CS_fsm_state34 === 1'b1) @(posedge clk);
                while (dut.ap_CS_fsm_state34 !== 1'b1) @(posedge clk);
            end
            @(negedge clk);
        end
    endtask

    initial begin
        reset_core();
        seed_initial_four();

        wait_requests(4);
        expect_request(0, TX0, 8'd0, ADDR0);
        expect_request(1, TX1, 8'd1, ADDR1);
        expect_request(2, TX2, 8'd2, ADDR2);
        expect_request(3, TX3, 8'd3, ADDR3);
        expect_lq_pending(0, TX0);
        expect_lq_pending(1, TX1);
        expect_lq_pending(2, TX2);
        expect_lq_pending(3, TX3);
        if (dut.state_lsu_outstanding_load_count_V !== 3'd4)
            $fatal(1, "G6_L3_RTL_FAIL max_outstanding got=%0d", dut.state_lsu_outstanding_load_count_V);
        $display("G6_L3_RTL_CASE_PASS case=max_issue_four requests=%0d", request_count);

        send_response(TX2, DATA2);
        wait_load_done(8'd2, 2, DATA2);
        if (dut.state_lsu_outstanding_load_count_V !== 3'd3)
            $fatal(1, "G6_L3_RTL_FAIL count_after_first_response got=%0d", dut.state_lsu_outstanding_load_count_V);
        expect_lq_pending(0, TX0);
        expect_lq_pending(1, TX1);
        expect_lq_pending(3, TX3);
        $display("G6_L3_RTL_CASE_PASS case=out_of_order_first txid=%h", TX2);

        seed_load(8'd4, 2, 32'h0000_0104, 16'h0024, ADDR4, 5'd5);
        dut.state_lsu_ldq_count_V = 3'd4;
        wait_requests(5);
        expect_request(4, TX4, 8'd4, ADDR4);
        expect_lq_pending(2, TX4);
        if (dut.state_lsu_outstanding_load_count_V !== 3'd4)
            $fatal(1, "G6_L3_RTL_FAIL count_after_reuse_issue got=%0d", dut.state_lsu_outstanding_load_count_V);
        $display("G6_L3_RTL_CASE_PASS case=reissue_after_free old_txid=%h new_txid=%h", TX2, TX4);

        send_response(TX2, 64'hfeed_face_cafe_beef);
        wait_architectural_cycles(2);
        expect_lq_pending(2, TX4);
        if (dut.state_rob_entries_memory_completed_U.ram[4] !== 1'b0)
            $fatal(1, "G6_L3_RTL_FAIL stale_old_txid_completed_new_owner");
        if (dut.state_lsu_outstanding_load_count_V !== 3'd4)
            $fatal(1, "G6_L3_RTL_FAIL stale_old_txid_count got=%0d", dut.state_lsu_outstanding_load_count_V);
        $display("G6_L3_RTL_CASE_PASS case=stale_old_txid_reject txid=%h", TX2);

        send_response(TX0, DATA0);
        wait_load_done(8'd0, 0, DATA0);
        send_response(TX4, DATA4);
        wait_load_done(8'd4, 2, DATA4);
        send_response(TX3, DATA3);
        wait_load_done(8'd3, 3, DATA3);
        send_response(TX1, DATA1);
        wait_load_done(8'd1, 1, DATA1);
        if (dut.state_lsu_outstanding_load_count_V !== 3'd0)
            $fatal(1, "G6_L3_RTL_FAIL final_pending_state count=%0d",
                   dut.state_lsu_outstanding_load_count_V);
        if (request_count !== 5 || response_count !== 6)
            $fatal(1, "G6_L3_RTL_FAIL transaction_count requests=%0d responses=%0d",
                   request_count, response_count);
        $display("G6_L3_RTL_CASE_PASS case=remaining_out_of_order_complete");
        $display("G6_L3_RTL_CONFIG_4_4_PASS cases=5 requests=%0d responses=%0d", request_count, response_count);
        $finish;
    end
endmodule
