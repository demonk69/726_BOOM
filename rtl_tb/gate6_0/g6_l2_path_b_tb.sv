`timescale 1ns/1ps

module g6_l2_path_b_tb;
    reg clk = 0;
    reg rst_n = 0;
    wire [191:0] imem_req_tdata;
    wire imem_req_tvalid;
    wire [319:0] dmem_req_tdata;
    wire dmem_req_tvalid;
    wire [767:0] trace_tdata;
    wire trace_tvalid;
    wire ap_local_block, ap_local_deadlock;
    wire io_success, io_halted, io_trap, io_cycle_valid;
    wire [63:0] io_cycle, io_instret;
    integer cycles = 0;
    integer dmem_requests = 0;
    integer forwarded_events = 0;
    integer architectural_cycles = 0;
    reg previous_forwarded = 0;

    localparam [63:0] STORE_ADDRESS = 64'h0000_0000_0000_1000;
    localparam [63:0] STORE_DATA = 64'h8877_6655_4433_2211;
    localparam [63:0] EXPECTED_VALUE = 64'h0000_0000_0000_5544;

    boom_core_top dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk), .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata), .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(1'b0), .imem_resp_in_TDATA(256'd0),
        .imem_resp_in_TVALID(1'b0), .imem_resp_in_TREADY(),
        .dmem_req_out_TDATA(dmem_req_tdata), .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(1'b0), .dmem_resp_in_TDATA(384'd0),
        .dmem_resp_in_TVALID(1'b0), .dmem_resp_in_TREADY(),
        .commit_trace_out_TDATA(trace_tdata), .commit_trace_out_TVALID(trace_tvalid),
        .commit_trace_out_TREADY(1'b1), .io_success(io_success),
        .io_halted(io_halted), .io_trap(io_trap),
        .io_cycle_valid(io_cycle_valid), .io_cycle(io_cycle), .io_instret(io_instret));

    always #5 clk = ~clk;

    always @(posedge clk) begin
        cycles <= cycles + 1;
        if (cycles > 120000) $fatal(1, "G6_L2_PATH_B_FAIL timeout");
        if (rst_n && dmem_req_tvalid) dmem_requests <= dmem_requests + 1;
        if (!rst_n) begin
            previous_forwarded <= 1'b0;
        end else begin
            if (dut.state_completion_load_response_valid &&
                dut.state_completion_load_response_forwarded_load && !previous_forwarded)
                forwarded_events <= forwarded_events + 1;
            previous_forwarded <= dut.state_completion_load_response_valid &&
                                  dut.state_completion_load_response_forwarded_load;
        end
        if (rst_n && dut.ap_CS_fsm_state34) architectural_cycles <= architectural_cycles + 1;
    end

    task automatic wait_reset_complete;
        begin
            while (dut.reset_ctrl_completed !== 1'b1) @(posedge clk);
            while (dut.ap_CS_fsm_state34 !== 1'b1) @(posedge clk);
            @(negedge clk);
        end
    endtask

    task automatic reset_core;
        begin
            rst_n = 1'b0;
            repeat (5) @(negedge clk);
            rst_n = 1'b1;
            wait_reset_complete();
        end
    endtask

    task automatic seed_forward_case(input [7:0] store_mask,
                                     input [31:0] store_allocation);
        begin
            dut.state_rob_head = 5'd0;
            dut.state_rob_tail = 5'd2;
            dut.state_rob_maybe_full = 1'b0;

            dut.state_rob_entries_valid_U.ram[0] = 1'b1;
            dut.state_rob_entries_busy_U.ram[0] = 1'b1;
            dut.state_rob_entries_uop_iq_type_U.ram[0] = 1'b1;
            dut.state_rob_entries_memory_valid_U.ram[0] = 1'b1;
            dut.state_rob_entries_is_store_U.ram[0] = 1'b1;
            dut.state_rob_entries_uop_imm_packed_5_U.ram[0] = 8'd0;
            dut.state_rob_entries_uop_imm_packed_U.ram[0] = 16'h0010;
            dut.state_rob_entries_uop_imm_packed_4_U.ram[0] = 32'h0000_0100;

            dut.state_lsu_stq_valid_U.ram[0] = 1'b1;
            dut.state_lsu_stq_address_valid_U.ram[0] = 1'b1;
            dut.state_lsu_stq_data_valid_U.ram[0] = 1'b1;
            dut.state_lsu_stq_generation_U.ram[0] = 16'h0010;
            dut.state_lsu_stq_rob_idx_U.ram[0] = 8'd0;
            dut.state_lsu_stq_rob_allocation_id_U.ram[0] = store_allocation;
            dut.state_lsu_stq_address_U.ram[0] = STORE_ADDRESS;
            dut.state_lsu_stq_data_U.ram[0] = STORE_DATA;
            dut.state_lsu_stq_mask_U.ram[0] = store_mask;
            dut.state_lsu_stq_count_V = 4'd1;

            dut.state_rob_entries_valid_U.ram[1] = 1'b1;
            dut.state_rob_entries_busy_U.ram[1] = 1'b1;
            dut.state_rob_entries_is_load_U.ram[1] = 1'b1;
            dut.state_rob_entries_memory_valid_U.ram[1] = 1'b1;
            dut.state_rob_entries_memory_request_sent_U.ram[1] = 1'b0;
            dut.state_rob_entries_memory_completed_U.ram[1] = 1'b0;
            dut.state_rob_entries_memory_address_U.ram[1] = STORE_ADDRESS + 64'd3;
            dut.state_rob_entries_memory_mask_U.ram[1] = 8'h03;
            dut.state_rob_entries_memory_size_U.ram[1] = 8'd1;
            dut.state_rob_entries_signed_load_U.ram[1] = 1'b0;
            dut.state_rob_entries_uop_imm_packed_2_U.ram[1] = 5'd1;
            dut.state_rob_entries_uop_imm_packed_1_U.ram[1] = 8'd0;
            dut.state_rob_entries_uop_imm_packed_3_U.ram[1] = 16'h0020;
            dut.state_rob_entries_uop_imm_packed_4_U.ram[1] = 32'h0000_0200;

            dut.state_lsu_ldq_valid_U.ram[0] = 1'b1;
            dut.state_lsu_ldq_response_pending_U.ram[0] = 1'b0;
            dut.state_lsu_ldq_generation_U.ram[0] = 16'h0020;
            dut.state_lsu_ldq_rob_idx_U.ram[0] = 8'd1;
            dut.state_lsu_ldq_rob_allocation_id_U.ram[0] = 32'h0000_0200;
            dut.state_lsu_ldq_count_V = 4'd1;
            dut.state_lsu_load_response_pending = 1'b0;
            dut.state_completion_load_response_valid = 1'b0;
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
        integer requests_before;
        integer forwards_before;

        reset_core();
        requests_before = dmem_requests;
        forwards_before = forwarded_events;
        seed_forward_case(8'hff, 32'h0000_0100);
        while (forwarded_events == forwards_before) @(posedge clk);
        while (dut.state_rob_entries_memory_completed_U.ram[1] !== 1'b1) @(posedge clk);
        wait_architectural_cycles(1);
        if (dmem_requests != requests_before) $fatal(1, "G6_L2_PATH_B_FAIL forwarded_load_reached_dmem");
        if (dut.state_lsu_load_response_pending !== 1'b0) $fatal(1, "G6_L2_PATH_B_FAIL external_pending_created");
        if (dut.state_rob_entries_memory_request_sent_U.ram[1] !== 1'b0) $fatal(1, "G6_L2_PATH_B_FAIL request_sent_set");
        if (dut.state_rob_entries_memory_data_U.ram[1] !== EXPECTED_VALUE) $fatal(1, "G6_L2_PATH_B_FAIL value_mismatch");
        if (dut.state_rob_entries_busy_U.ram[1] !== 1'b0) $fatal(1, "G6_L2_PATH_B_FAIL rob_not_completed");
        if (dut.state_lsu_ldq_valid_U.ram[0] !== 1'b0 || dut.state_lsu_ldq_count_V !== 0) begin
            $display("G6_L2_PATH_B_DIAG lq_valid=%b lq_count=%0d generation=%h rob=%h allocation=%h",
                     dut.state_lsu_ldq_valid_U.ram[0], dut.state_lsu_ldq_count_V,
                     dut.state_lsu_ldq_generation_U.ram[0], dut.state_lsu_ldq_rob_idx_U.ram[0],
                     dut.state_lsu_ldq_rob_allocation_id_U.ram[0]);
            $fatal(1, "G6_L2_PATH_B_FAIL lq_not_reclaimed");
        end
        $display("G6_L2_PATH_B_CASE_PASS case=forward_lifecycle value=%016x", EXPECTED_VALUE);

        reset_core();
        requests_before = dmem_requests;
        forwards_before = forwarded_events;
        seed_forward_case(8'h08, 32'h0000_0100);
        wait_architectural_cycles(1);
        if (dmem_requests != requests_before || forwarded_events != forwards_before)
            $fatal(1, "G6_L2_PATH_B_FAIL partial_overlap_not_blocked");
        if (dut.state_rob_entries_busy_U.ram[1] !== 1'b1)
            $fatal(1, "G6_L2_PATH_B_FAIL partial_overlap_completed");
        $display("G6_L2_PATH_B_CASE_PASS case=partial_overlap_block");

        reset_core();
        requests_before = dmem_requests;
        forwards_before = forwarded_events;
        seed_forward_case(8'hff, 32'hdead_beef);
        wait_architectural_cycles(1);
        if (dmem_requests != requests_before || forwarded_events != forwards_before)
            $fatal(1, "G6_L2_PATH_B_FAIL reused_store_owner_not_blocked");
        $display("G6_L2_PATH_B_CASE_PASS case=store_owner_reuse_block");

        reset_core();
        requests_before = dmem_requests;
        forwards_before = forwarded_events;
        seed_forward_case(8'hff, 32'h0000_0100);
        dut.state_lsu_ldq_generation_U.ram[0] = 16'hffff;
        wait_architectural_cycles(1);
        if (dmem_requests != requests_before || forwarded_events != forwards_before ||
            dut.state_rob_entries_busy_U.ram[1] !== 1'b1)
            $fatal(1, "G6_L2_PATH_B_FAIL stale_lq_generation_not_blocked");
        $display("G6_L2_PATH_B_CASE_PASS case=stale_lq_generation_block");

        reset_core();
        requests_before = dmem_requests;
        forwards_before = forwarded_events;
        seed_forward_case(8'hff, 32'h0000_0100);
        dut.state_lsu_ldq_rob_allocation_id_U.ram[0] = 32'hdead_beef;
        wait_architectural_cycles(1);
        if (dmem_requests != requests_before || forwarded_events != forwards_before ||
            dut.state_rob_entries_busy_U.ram[1] !== 1'b1)
            $fatal(1, "G6_L2_PATH_B_FAIL stale_lq_allocation_not_blocked");
        $display("G6_L2_PATH_B_CASE_PASS case=stale_lq_allocation_block");

        reset_core();
        requests_before = dmem_requests;
        forwards_before = forwarded_events;
        seed_forward_case(8'hff, 32'h0000_0100);
        while (forwarded_events == forwards_before) @(posedge clk);
        @(negedge clk);
        rst_n = 1'b0;
        repeat (5) @(negedge clk);
        rst_n = 1'b1;
        wait_reset_complete();
        if (dmem_requests != requests_before) $fatal(1, "G6_L2_PATH_B_FAIL reset_race_reached_dmem");
        if (dut.state_completion_load_response_valid !== 1'b0 ||
            dut.state_lsu_load_response_pending !== 1'b0 ||
            dut.state_lsu_ldq_count_V !== 0 || dut.state_lsu_stq_count_V !== 0)
            $fatal(1, "G6_L2_PATH_B_FAIL reset_race_state_not_cleared");
        $display("G6_L2_PATH_B_CASE_PASS case=forward_reset_race");
        $display("G6_L2_PATH_B_PASS cases=6 dmem_requests=%0d forwarded_events=%0d", dmem_requests, forwarded_events);
        $finish;
    end
endmodule
