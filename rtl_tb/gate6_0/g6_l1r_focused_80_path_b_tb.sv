`timescale 1ns/1ps

module g6_l1r_focused_80_path_b_tb;
    reg clk;
    reg rst_n;
    reg [7:0] scenario_code;
    reg allow_imem;
    reg manual_resp_valid;
    reg [383:0] manual_resp_data;
    integer case_id;
    integer cycles;
    integer max_cycles;
    integer i;

    wire [191:0] imem_req_tdata;
    wire imem_req_tvalid;
    wire imem_req_tready;
    wire model_imem_req_tready;
    wire [255:0] imem_resp_tdata;
    wire imem_resp_tvalid;
    wire imem_resp_tready;
    wire [319:0] dmem_req_tdata;
    wire dmem_req_tvalid;
    wire dmem_req_tready;
    wire [383:0] model_dmem_resp_tdata;
    wire model_dmem_resp_tvalid;
    wire model_dmem_resp_tready;
    wire [383:0] dmem_resp_tdata;
    wire dmem_resp_tvalid;
    wire dmem_resp_tready;
    wire [767:0] commit_trace_tdata;
    wire commit_trace_tvalid;
    wire commit_trace_tready;
    wire ap_local_block;
    wire ap_local_deadlock;
    wire io_success;
    wire io_halted;
    wire io_trap;
    wire io_cycle_valid;
    wire [63:0] io_cycle;
    wire [63:0] io_instret;
    wire [31:0] imem_requests;
    wire [31:0] imem_responses;
    wire [31:0] dmem_requests;
    wire [31:0] dmem_responses;
    wire [31:0] dmem_loads;
    wire [31:0] dmem_stores;
    wire tohost_seen;
    wire [63:0] tohost_value;
    wire imem_error;
    wire dmem_error;
    wire imem_pending;
    wire dmem_pending;

    wire request_is_store = dmem_req_tdata[88] || dmem_req_tdata[47:40] == 8'd1;
    wire request_committed = dmem_req_tdata[104];

    assign imem_req_tready = allow_imem && model_imem_req_tready;
    assign dmem_resp_tdata = manual_resp_valid ? manual_resp_data : model_dmem_resp_tdata;
    assign dmem_resp_tvalid = manual_resp_valid || model_dmem_resp_tvalid;
    assign model_dmem_resp_tready = dmem_resp_tready && !manual_resp_valid;
    assign commit_trace_tready = rst_n;

    boom_core_top dut (
        .ap_local_block(ap_local_block),
        .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk),
        .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata),
        .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(imem_req_tready),
        .imem_resp_in_TDATA(imem_resp_tdata),
        .imem_resp_in_TVALID(imem_resp_tvalid),
        .imem_resp_in_TREADY(imem_resp_tready),
        .dmem_req_out_TDATA(dmem_req_tdata),
        .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(dmem_req_tready),
        .dmem_resp_in_TDATA(dmem_resp_tdata),
        .dmem_resp_in_TVALID(dmem_resp_tvalid),
        .dmem_resp_in_TREADY(dmem_resp_tready),
        .commit_trace_out_TDATA(commit_trace_tdata),
        .commit_trace_out_TVALID(commit_trace_tvalid),
        .commit_trace_out_TREADY(commit_trace_tready),
        .io_success(io_success),
        .io_halted(io_halted),
        .io_trap(io_trap),
        .io_cycle_valid(io_cycle_valid),
        .io_cycle(io_cycle),
        .io_instret(io_instret)
    );

    axis_imem_model imem_model (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .req_tdata(imem_req_tdata), .req_tvalid(imem_req_tvalid),
        .req_tready(model_imem_req_tready), .resp_tdata(imem_resp_tdata),
        .resp_tvalid(imem_resp_tvalid), .resp_tready(imem_resp_tready),
        .request_count(imem_requests), .response_count(imem_responses),
        .protocol_error(imem_error), .pending_visible(imem_pending)
    );

    axis_dmem_model dmem_model (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .req_tdata(dmem_req_tdata), .req_tvalid(dmem_req_tvalid),
        .req_tready(dmem_req_tready), .resp_tdata(model_dmem_resp_tdata),
        .resp_tvalid(model_dmem_resp_tvalid), .resp_tready(model_dmem_resp_tready),
        .request_count(dmem_requests), .response_count(dmem_responses),
        .load_count(dmem_loads), .store_count(dmem_stores),
        .tohost_seen(tohost_seen), .tohost_value(tohost_value),
        .protocol_error(dmem_error), .pending_visible(dmem_pending)
    );

    task automatic fail(input string reason);
        begin
            $fatal(1, "G6_L1R80_PATH_B_FAIL id=%0d reason=%s cycles=%0d", case_id, reason, cycles);
        end
    endtask

    task automatic wait_reset_complete;
        begin
            while (dut.reset_ctrl_completed !== 1'b1) @(posedge clk);
            @(negedge clk);
        end
    endtask

    task automatic pulse_reset;
        begin
            @(negedge clk);
            rst_n = 1'b0;
            repeat (4) @(negedge clk);
            rst_n = 1'b1;
            wait_reset_complete();
        end
    endtask

    task automatic seed_queues(input integer lq_count, input integer stq_count,
                               input bit pending);
        begin
            @(negedge clk);
            dut.state_lsu_ldq_count_V = lq_count;
            dut.state_lsu_stq_count_V = stq_count;
            dut.state_lsu_ldq_tail_V = lq_count[2:0];
            dut.state_lsu_stq_tail_V = stq_count[2:0];
            dut.state_lsu_load_response_pending = pending;
            dut.state_lsu_pending_load_transaction_id = 32'h1357_2468;
            dut.state_lsu_pending_load_rob_idx = 5'd3;
            dut.state_lsu_pending_load_allocation_id = 32'h2468_1357;
            dut.state_lsu_pending_load_lq_index_V = 3'd0;
            dut.state_lsu_pending_load_lq_generation = 16'h1234;
            for (i = 0; i < 8; i = i + 1) begin
                dut.state_lsu_ldq_valid_U.ram[i] = i < lq_count;
                dut.state_lsu_ldq_response_pending_U.ram[i] = pending && i == 0;
                dut.state_lsu_ldq_generation_U.ram[i] = i < lq_count ? 16'h1234 + i : 0;
                dut.state_lsu_ldq_rob_idx_U.ram[i] = i < lq_count ? 8'd3 + i : 0;
                dut.state_lsu_ldq_rob_allocation_id_U.ram[i] = i < lq_count ? 32'h2468_1357 + i : 0;
                dut.state_lsu_ldq_transaction_id_U.ram[i] = i == 0 && pending ? 32'h1357_2468 : 0;
                dut.state_lsu_stq_valid_U.ram[i] = i < stq_count;
                dut.state_lsu_stq_generation_U.ram[i] = i < stq_count ? 16'h4321 + i : 0;
                dut.state_lsu_stq_rob_idx_U.ram[i] = i < stq_count ? 8'd12 + i : 0;
                dut.state_lsu_stq_rob_allocation_id_U.ram[i] = i < stq_count ? 32'h5555_0000 + i : 0;
            end
        end
    endtask

    task automatic require_empty(input string phase);
        begin
            if (dut.state_lsu_ldq_count_V !== 0 || dut.state_lsu_stq_count_V !== 0 ||
                dut.state_lsu_load_response_pending !== 0)
                fail({phase, "_counts_or_pending"});
            for (i = 0; i < 8; i = i + 1) begin
                if (dut.state_lsu_ldq_valid_U.ram[i] !== 0 ||
                    dut.state_lsu_ldq_response_pending_U.ram[i] !== 0 ||
                    dut.state_lsu_stq_valid_U.ram[i] !== 0)
                    fail({phase, "_valid_owner"});
            end
        end
    endtask

    task automatic wait_program_pass;
        begin
            while (!(tohost_seen && tohost_value == 64'd1)) @(posedge clk);
            if (imem_error || dmem_error) fail("axis_protocol_error");
        end
    endtask

    task automatic stale_issue_case(input bit corrupt_allocation);
        integer loads_before;
        begin
            allow_imem = 1'b1;
            while (!(dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.ap_CS_fsm_state5 &&
                     dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_rob_entries_valid_load_reg_640 &&
                     dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_rob_entries_is_load_load_reg_649 &&
                     dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_rob_entries_memory_valid_load_reg_658))
                @(posedge clk);
            loads_before = dmem_loads;
            if (corrupt_allocation)
                force dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_lsu_ldq_rob_allocation_id_q0 = 32'hdead_beef;
            else
                force dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_lsu_ldq_generation_q0 = 16'hffff;
            while (!dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.ap_CS_fsm_state14)
                @(posedge clk);
            if (dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.pipe_2_write)
                fail("stale_owner_issued_request");
            if (dmem_loads != loads_before) fail("stale_owner_reached_axis");
            if (corrupt_allocation)
                release dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_lsu_ldq_rob_allocation_id_q0;
            else
                release dut.grp_boom_core_cycle_io_fu_6225.grp_try_issue_load_fu_3481.state_lsu_ldq_generation_q0;
        end
    endtask

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    always @(posedge clk) begin
        cycles <= cycles + 1;
        if (cycles >= max_cycles) fail("timeout");
    end

    initial begin : RUN_CASE
        reg [319:0] held_request;
        integer loads_at_store_stall;
        integer stores_at_stall;

        if (!$value$plusargs("CASE_ID=%d", case_id)) fail("missing_case_id");
        if (!$value$plusargs("MAX_CYCLES=%d", max_cycles)) max_cycles = 60000;
        case (case_id)
            13, 47: scenario_code = 8'd30;
            28: scenario_code = 8'd32;
            29: scenario_code = 8'd38;
            36: scenario_code = 8'd35;
            48: scenario_code = 8'd46;
            49: scenario_code = 8'd4;
            default: scenario_code = 8'd0;
        endcase
        cycles = 0;
        rst_n = 1'b0;
        allow_imem = 1'b0;
        manual_resp_valid = 1'b0;
        manual_resp_data = 0;
        repeat (5) @(negedge clk);
        rst_n = 1'b1;
        wait_reset_complete();

        case (case_id)
            13: begin
                stale_issue_case(1'b0);
                $display("G6_L1R80_PATH_B_PASS id=13 actual=generation_mismatch_rejected loads=%0d", dmem_loads);
            end
            28: begin
                allow_imem = 1'b1;
                while (!(dmem_req_tvalid && !dmem_req_tready && !request_is_store)) @(posedge clk);
                held_request = dmem_req_tdata;
                @(posedge clk);
                if (!dmem_req_tvalid || dmem_req_tdata !== held_request)
                    fail("load_request_not_held_under_backpressure");
                while (!(dmem_req_tvalid && dmem_req_tready && !request_is_store)) @(posedge clk);
                wait_program_pass();
                $display("G6_L1R80_PATH_B_PASS id=28 actual=load_stalled_then_drained loads=%0d", dmem_loads);
            end
            29: begin
                allow_imem = 1'b1;
                while (!(dmem_req_tvalid && !dmem_req_tready && request_is_store && request_committed)) @(posedge clk);
                held_request = dmem_req_tdata;
                stores_at_stall = dmem_stores;
                @(posedge clk);
                if (!dmem_req_tvalid || dmem_req_tdata !== held_request || dmem_stores != stores_at_stall)
                    fail("committed_store_changed_or_reclaimed_while_blocked");
                while (!(dmem_req_tvalid && dmem_req_tready && request_is_store)) @(posedge clk);
                wait_program_pass();
                $display("G6_L1R80_PATH_B_PASS id=29 actual=committed_store_held_then_reclaimed stores=%0d", dmem_stores);
            end
            36: begin
                seed_queues(2, 2, 1'b1);
                allow_imem = 1'b1;
                while (dut.state_exception_commit_valid !== 1'b1 && dut.state_global_flush !== 1'b1) @(posedge clk);
                while (dut.state_global_flush !== 1'b1) @(posedge clk);
                while (dut.state_lsu_ldq_count_V != 0 || dut.state_lsu_stq_count_V != 0 ||
                       dut.state_lsu_load_response_pending != 0) @(posedge clk);
                manual_resp_data = {64'd0, 64'd0, 63'd0, 1'b0,
                                    64'hfeed_face_dead_beef, 64'hfeed_face_dead_beef,
                                    32'd0, 32'h1357_2468};
                manual_resp_valid = 1'b1;
                while (!dmem_resp_tready) @(posedge clk);
                @(negedge clk);
                manual_resp_valid = 1'b0;
                repeat (80) @(posedge clk);
                require_empty("exception_late_response");
                if (dut.state_completion_load_response_valid)
                    fail("late_response_completed_after_exception");
                $display("G6_L1R80_PATH_B_PASS id=36 actual=exception_flush_cleared_queues_late_response_rejected");
            end
            38: begin
                require_empty("reset_empty");
                $display("G6_L1R80_PATH_B_PASS id=38 actual=staged_reset_empty_completed");
            end
            39: begin
                seed_queues(3, 2, 1'b0);
                if (dut.state_lsu_ldq_count_V != 3 || dut.state_lsu_stq_count_V != 2)
                    fail("partial_seed_failed");
                pulse_reset();
                require_empty("reset_partial");
                $display("G6_L1R80_PATH_B_PASS id=39 actual=staged_reset_partial_cleared lq=3 sq=2");
            end
            40: begin
                seed_queues(2, 8, 1'b0);
                if (dut.state_lsu_ldq_count_V != 2 || dut.state_lsu_stq_count_V != 8)
                    fail("full_seed_failed");
                pulse_reset();
                require_empty("reset_full");
                $display("G6_L1R80_PATH_B_PASS id=40 actual=staged_reset_full_cleared lq=2 sq=8");
            end
            47: begin
                stale_issue_case(1'b1);
                $display("G6_L1R80_PATH_B_PASS id=47 actual=rob_allocation_mismatch_rejected loads=%0d", dmem_loads);
            end
            48: begin
                allow_imem = 1'b1;
                while (!(dmem_req_tvalid && !dmem_req_tready && request_is_store && request_committed)) @(posedge clk);
                held_request = dmem_req_tdata;
                loads_at_store_stall = dmem_loads;
                @(posedge clk);
                if (!dmem_req_tvalid || dmem_req_tdata !== held_request || dmem_loads != loads_at_store_stall)
                    fail("younger_load_advanced_during_committed_store_stall");
                while (!(dmem_req_tvalid && dmem_req_tready && request_is_store)) @(posedge clk);
                while (dmem_loads == loads_at_store_stall) @(posedge clk);
                wait_program_pass();
                $display("G6_L1R80_PATH_B_PASS id=48 actual=store_accept_preceded_younger_load stores=%0d loads=%0d",
                         dmem_stores, dmem_loads);
            end
            49: begin
                allow_imem = 1'b1;
                while (!(dut.state_lsu_load_response_pending && dmem_pending)) @(posedge clk);
                pulse_reset();
                require_empty("reset_pending");
                wait_program_pass();
                require_empty("reset_pending_complete");
                $display("G6_L1R80_PATH_B_PASS id=49 actual=reset_cleared_pending_and_restarted responses=%0d", dmem_responses);
            end
            default: fail("unsupported_case_id");
        endcase

        if (imem_error || dmem_error) fail("axis_protocol_error_at_finish");
        $finish;
    end
endmodule
