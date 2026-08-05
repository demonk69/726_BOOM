`timescale 1ns/1ps

module divider_core_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg reset = 0;
    reg request_valid = 0;
    reg [7:0] operation = 0;
    reg [63:0] dividend = 0;
    reg [63:0] divisor = 0;
    reg [7:0] rob_idx = 0;
    reg [31:0] allocation_id = 0;
    reg [7:0] pdst = 0;
    reg [7:0] branch_mask = 0;
    reg completion_ready = 0;
    reg branch_kill = 0;
    reg [7:0] kill_mask = 0;
    reg stale_owner = 0;
    reg [7:0] int_collision = 0;

    wire ap_done;
    wire ap_idle;
    wire ap_ready;
    wire request_accepted;
    wire request_accepted_ap_vld;
    wire busy;
    wire busy_ap_vld;
    wire response_pending;
    wire response_pending_ap_vld;
    wire token_valid;
    wire token_valid_ap_vld;
    wire int_result_valid;
    wire int_result_valid_ap_vld;
    wire [63:0] result;
    wire result_ap_vld;
    wire writeback_valid;
    wire writeback_valid_ap_vld;
    wire rob_complete;
    wire rob_complete_ap_vld;
    wire [31:0] active_allocation;
    wire active_allocation_ap_vld;

    integer pass_count = 0;
    integer next_rob = 1;
    integer modeled_cycles = 0;

    synth_m3b_divider_core_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .reset(reset), .request_valid(request_valid), .operation(operation),
        .dividend(dividend), .divisor(divisor), .rob_idx(rob_idx),
        .allocation_id(allocation_id), .pdst(pdst),
        .branch_mask(branch_mask), .completion_ready(completion_ready),
        .branch_kill(branch_kill), .kill_mask(kill_mask),
        .stale_owner(stale_owner), .int_collision(int_collision),
        .request_accepted(request_accepted),
        .request_accepted_ap_vld(request_accepted_ap_vld), .busy(busy),
        .busy_ap_vld(busy_ap_vld), .response_pending(response_pending),
        .response_pending_ap_vld(response_pending_ap_vld),
        .token_valid(token_valid), .token_valid_ap_vld(token_valid_ap_vld),
        .int_result_valid(int_result_valid),
        .int_result_valid_ap_vld(int_result_valid_ap_vld), .result(result),
        .result_ap_vld(result_ap_vld), .writeback_valid(writeback_valid),
        .writeback_valid_ap_vld(writeback_valid_ap_vld),
        .rob_complete(rob_complete), .rob_complete_ap_vld(rob_complete_ap_vld),
        .active_allocation(active_allocation),
        .active_allocation_ap_vld(active_allocation_ap_vld),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready)
    );

    always #5 ap_clk = ~ap_clk;

    task fail(input [8*64-1:0] name, input [8*160-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "M3B_RTL_FAIL case=%0s reason=%0s", name, reason);
        end
    endtask

    task pass(input [8*64-1:0] name, input [8*160-1:0] requirement);
        begin
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task cycle;
        input do_reset;
        input do_request;
        input [7:0] op;
        input [63:0] lhs;
        input [63:0] rhs;
        input [7:0] ridx;
        input [31:0] alloc;
        input [7:0] dest;
        input [7:0] brmask;
        input complete;
        input do_kill;
        input [7:0] kmask;
        input do_stale;
        input [7:0] collision;
        begin
            @(negedge ap_clk);
            reset = do_reset;
            request_valid = do_request;
            operation = op;
            dividend = lhs;
            divisor = rhs;
            rob_idx = ridx;
            allocation_id = alloc;
            pdst = dest;
            branch_mask = brmask;
            completion_ready = complete;
            branch_kill = do_kill;
            kill_mask = kmask;
            stale_owner = do_stale;
            int_collision = collision;
            ap_start = 1;
            @(posedge ap_clk);
            @(negedge ap_clk);
            ap_start = 0;
            wait (ap_done === 1'b1);
            if (!(request_accepted_ap_vld && busy_ap_vld &&
                  response_pending_ap_vld && token_valid_ap_vld &&
                  int_result_valid_ap_vld && result_ap_vld &&
                  writeback_valid_ap_vld && rob_complete_ap_vld &&
                  active_allocation_ap_vld))
                fail("generated_output_protocol", "an output validity strobe was absent at ap_done");
            modeled_cycles = modeled_cycles + 1;
            @(negedge ap_clk);
        end
    endtask

    task clean_reset;
        begin
            cycle(1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
            if (request_accepted || busy || response_pending || token_valid ||
                int_result_valid || writeback_valid || rob_complete ||
                active_allocation != 0)
                fail("clean_reset", "diagnostic state did not clear");
        end
    endtask

    task run_arithmetic;
        input [8*64-1:0] name;
        input [7:0] op;
        input [63:0] lhs;
        input [63:0] rhs;
        input [63:0] expected;
        integer guard;
        integer ridx;
        integer alloc;
        begin
            clean_reset();
            ridx = next_rob;
            alloc = 1000 + next_rob;
            next_rob = (next_rob == 30) ? 1 : next_rob + 1;
            cycle(0, 1, op, lhs, rhs, ridx, alloc, ridx + 10, 0,
                  1, 0, 0, 0, 0);
            if (!request_accepted || !token_valid || active_allocation != alloc)
                fail(name, "request or ownership token was not accepted");
            guard = 0;
            while (!writeback_valid && guard < 70) begin
                cycle(0, 0, 0, 0, 0, ridx, alloc, ridx + 10, 0,
                      1, 0, 0, 0, 0);
                guard = guard + 1;
            end
            if (!writeback_valid || !rob_complete)
                fail(name, "result did not reach writeback and ROB completion");
            if (result !== expected)
                fail(name, "arithmetic result mismatch");
            if (token_valid || response_pending || busy)
                fail(name, "divider ownership remained after completion");
            pass(name, "accepted operation produced exact result and one observed writeback/ROB completion");
        end
    endtask

    task advance_to_response;
        input [7:0] ridx;
        input [31:0] alloc;
        integer guard;
        begin
            cycle(0, 1, 1, 64'd1000, 64'd7, ridx, alloc, 20, 1,
                  0, 0, 0, 0, 0);
            if (!request_accepted || !token_valid)
                fail("advance_to_response", "normal divide was not accepted");
            guard = 0;
            while (!response_pending && guard < 65) begin
                cycle(0, 0, 0, 0, 0, ridx, alloc, 20, 1,
                      0, 0, 0, 0, 0);
                guard = guard + 1;
            end
            if (!response_pending || !token_valid || busy)
                fail("advance_to_response", "response did not become pending");
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        ap_rst = 0;

        run_arithmetic("normal_div_signed", 0, -64'sd100, 64'd7,
                       64'hffff_ffff_ffff_fff2);
        run_arithmetic("normal_div_unsigned", 1, 64'd100, 64'd7, 64'd14);
        run_arithmetic("normal_rem_signed", 2, -64'sd100, 64'd7,
                       64'hffff_ffff_ffff_fffe);
        run_arithmetic("normal_rem_unsigned", 3, 64'd100, 64'd7, 64'd2);
        run_arithmetic("normal_divw_signed", 4, 64'hffff_ffff_ffff_ff9c,
                       64'd7, 64'hffff_ffff_ffff_fff2);
        run_arithmetic("normal_divw_unsigned", 5, 64'h1234_5678_0000_0064,
                       64'd7, 64'd14);
        run_arithmetic("normal_remw_signed", 6, 64'hffff_ffff_ffff_ff9c,
                       64'd7, 64'hffff_ffff_ffff_fffe);
        run_arithmetic("normal_remw_unsigned", 7, 64'h1234_5678_0000_0064,
                       64'd7, 64'd2);
        run_arithmetic("divide_zero_quotient", 0, 64'h8000_0000_0000_0001,
                       0, 64'hffff_ffff_ffff_ffff);
        run_arithmetic("divide_zero_remainder", 2, 64'h8000_0000_0000_0001,
                       0, 64'h8000_0000_0000_0001);
        run_arithmetic("signed_overflow_quotient", 0, 64'h8000_0000_0000_0000,
                       64'hffff_ffff_ffff_ffff, 64'h8000_0000_0000_0000);
        run_arithmetic("signed_overflow_remainder", 2, 64'h8000_0000_0000_0000,
                       64'hffff_ffff_ffff_ffff, 0);

        clean_reset();
        advance_to_response(4, 2004);
        cycle(0, 0, 0, 9, 4, 4, 2004, 20, 1, 0, 0, 0, 0, 1);
        if (!response_pending || !token_valid || !int_result_valid || result != 13)
            fail("response_hold", "pending response was not retained behind occupied INT result");
        pass("response_hold", "pending divider response held with ownership while INT result occupied");
        if (!response_pending || !int_result_valid)
            fail("existing_source_collision", "equivalent existing-source coexistence was absent");
        pass("existing_source_collision", "pending divider coexisted with an existing INT-source result");
        cycle(0, 0, 0, 0, 0, 4, 2004, 20, 1, 1, 0, 0, 0, 0);
        if (!response_pending || !token_valid || !writeback_valid || result != 13)
            fail("alu_collision", "ALU result did not win before held divider response");
        pass("alu_collision", "ALU collision completed first and retained divider response");
        cycle(0, 0, 0, 0, 0, 4, 2004, 20, 1, 1, 0, 0, 0, 0);
        if (!writeback_valid || !rob_complete || result != 64'd142 || token_valid)
            fail("dependent_writeback_observation", "divider result was not observable at writeback");
        pass("dependent_writeback_observation", "held divider result became observable on writeback and ROB complete");
        cycle(0, 0, 0, 0, 0, 4, 2004, 20, 1, 1, 0, 0, 0, 0);
        if (writeback_valid || rob_complete || int_result_valid)
            fail("no_duplicate_complete", "completed divider emitted a duplicate event");
        pass("no_duplicate_complete", "cycle after consumption had no duplicate completion or writeback");

        clean_reset();
        cycle(0, 1, 1, 100, 7, 5, 3005, 21, 0, 0, 0, 0, 0, 0);
        if (!request_accepted || !token_valid || !busy)
            fail("reset_active", "iterative divider was not active before reset");
        cycle(1, 0, 0, 0, 0, 5, 3005, 21, 0, 1, 0, 0, 0, 0);
        if (token_valid || busy || response_pending || writeback_valid || rob_complete)
            fail("reset_active", "reset did not discard active divider");
        pass("reset_active", "runtime reset discarded an active divider without side effect");

        clean_reset();
        advance_to_response(6, 3006);
        cycle(1, 0, 0, 0, 0, 6, 3006, 22, 1, 1, 0, 0, 0, 0);
        if (token_valid || busy || response_pending || writeback_valid || rob_complete)
            fail("reset_pending", "reset did not discard pending response without side effect");
        pass("reset_pending", "modeled reset discarded a pending response and ownership token");

        cycle(0, 1, 1, 81, 9, 11, 3011, 27, 0, 1, 0, 0, 0, 0);
        while (!writeback_valid) cycle(0, 0, 0, 0, 0, 11, 3011, 27, 0, 1, 0, 0, 0, 0);
        if (!rob_complete || result != 9 || token_valid)
            fail("reset_post_reexecute", "new divide did not complete after runtime reset");
        pass("reset_post_reexecute", "a fresh divider token completed after runtime reset");

        clean_reset();
        cycle(0, 1, 1, 1000, 7, 12, 3012, 28, 0, 0, 0, 0, 0, 0);
        cycle(1, 0, 0, 0, 0, 12, 3012, 28, 0, 1, 0, 0, 0, 0);
        cycle(0, 1, 1, 999, 9, 13, 3013, 29, 0, 0, 0, 0, 0, 0);
        cycle(1, 0, 0, 0, 0, 13, 3013, 29, 0, 1, 0, 0, 0, 0);
        if (token_valid || busy || response_pending || writeback_valid || rob_complete)
            fail("double_runtime_reset_divider", "second runtime reset retained divider work");
        pass("double_runtime_reset_divider", "two runtime resets independently discarded divider work");

        cycle(0, 0, 0, 0, 0, 13, 3013, 29, 0, 1, 0, 0, 1, 0);
        if (token_valid || response_pending || writeback_valid || rob_complete)
            fail("reset_stale_completion", "pre-reset allocation produced a stale side effect");
        pass("reset_stale_completion", "post-reset stale allocation could not produce completion");

        clean_reset();
        cycle(0, 1, 1, 1000, 7, 7, 3007, 23, 4, 0, 0, 0, 0, 0);
        if (!request_accepted || !token_valid || !busy)
            fail("branch_kill", "branch-tagged divide did not start");
        cycle(0, 0, 0, 0, 0, 7, 3007, 23, 4, 1, 1, 4, 0, 0);
        if (token_valid || busy || response_pending || writeback_valid || rob_complete)
            fail("branch_kill", "matching branch kill did not cancel divider cleanly");
        pass("branch_kill", "matching mispredict mask canceled active divider without completion");

        clean_reset();
        cycle(0, 1, 1, 1000, 7, 8, 3008, 24, 0, 0, 0, 0, 0, 0);
        if (!request_accepted || !token_valid)
            fail("stale_allocation", "divide did not establish allocation ownership");
        cycle(0, 0, 0, 0, 0, 8, 3008, 24, 0, 1, 0, 0, 1, 0);
        if (token_valid || busy || response_pending || writeback_valid || rob_complete)
            fail("stale_allocation", "allocation mismatch did not cancel divider");
        pass("stale_allocation", "stale ROB allocation ownership canceled without architectural side effect");

        clean_reset();
        cycle(0, 1, 1, 1000, 7, 9, 3009, 25, 0, 0, 0, 0, 0, 0);
        cycle(0, 0, 0, 11, 5, 9, 3009, 25, 0, 0, 0, 0, 0, 1);
        if (!token_valid || !busy || !int_result_valid || result != 16)
            fail("alu_while_divider_busy", "ALU did not coexist while divider advanced");
        pass("alu_while_divider_busy", "ALU result occupied INT source while iterative divider remained active");

        clean_reset();
        cycle(0, 1, 1, 1000, 7, 10, 3010, 26, 0, 0, 0, 0, 0, 0);
        cycle(0, 0, 0, 6, 7, 10, 3010, 26, 0, 0, 0, 0, 0, 2);
        if (!token_valid || !busy || !int_result_valid || result != 42)
            fail("mul_collision", "MUL did not coexist while divider advanced");
        pass("mul_collision", "MUL result occupied INT source while iterative divider remained active");

        if (pass_count != 26)
            fail("matrix_cardinality", "expected exactly 26 asserted named cases");
        $display("M3B_DIVIDER_RTL_PASS cases=%0d modeled_cycles=%0d", pass_count, modeled_cycles);
        $finish;
    end

    initial begin
        repeat (5000000) @(posedge ap_clk);
        $fatal(1, "M3B_RTL_TIMEOUT modeled_cycles=%0d", modeled_cycles);
    end
endmodule
