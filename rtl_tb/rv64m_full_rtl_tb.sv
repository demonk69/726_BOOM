`timescale 1ns/1ps

module rv64m_full_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg reset = 0, request_valid = 0, completion_ready = 0;
    reg branch_kill = 0, stale_owner = 0;
    reg [7:0] uopc = 0, rob_idx = 0, pdst = 0;
    reg [7:0] branch_mask = 0, kill_mask = 0;
    reg [31:0] allocation_id = 0;
    reg [63:0] lhs = 0, rhs = 0;
    wire ap_done, ap_idle, ap_ready;
    wire request_accepted, divider_busy, response_pending, token_valid;
    wire int_result_valid, writeback_valid, rob_complete, lane2_inactive;
    wire [63:0] result;
    wire [7:0] prf_writes, wakeups, bypasses;
    wire [31:0] active_allocation;
    integer pass_count = 0;
    integer next_id = 100;
    reg txn_accepted, txn_writeback, txn_rob_complete;
    reg [63:0] txn_result;
    reg [7:0] txn_prf_writes, txn_wakeups, txn_bypasses;

    synth_m3c_rv64m_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .reset(reset), .request_valid(request_valid), .uopc(uopc),
        .lhs(lhs), .rhs(rhs), .rob_idx(rob_idx), .allocation_id(allocation_id),
        .pdst(pdst), .branch_mask(branch_mask), .completion_ready(completion_ready),
        .branch_kill(branch_kill), .kill_mask(kill_mask), .stale_owner(stale_owner),
        .request_accepted(request_accepted), .divider_busy(divider_busy),
        .response_pending(response_pending), .token_valid(token_valid),
        .int_result_valid(int_result_valid), .result(result),
        .writeback_valid(writeback_valid), .rob_complete(rob_complete),
        .prf_writes(prf_writes), .wakeups(wakeups), .bypasses(bypasses),
        .lane2_inactive(lane2_inactive), .active_allocation(active_allocation),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready)
    );

    always #5 ap_clk = ~ap_clk;

    task fail(input [8*64-1:0] name, input [8*160-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "M3C_RTL_FAIL case=%0s reason=%0s", name, reason);
        end
    endtask

    task pass(input [8*64-1:0] name, input [8*160-1:0] requirement);
        begin
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task cycle;
        input do_reset, do_request;
        input [7:0] op;
        input [63:0] a, b;
        input [7:0] ridx;
        input [31:0] alloc;
        input [7:0] dest, brmask;
        input complete, do_kill;
        input [7:0] kmask;
        input do_stale;
        integer guard;
        begin
            txn_accepted = 0; txn_writeback = 0; txn_rob_complete = 0;
            txn_result = 0; txn_prf_writes = 0; txn_wakeups = 0; txn_bypasses = 0;
            guard = 0;
            while (ap_idle !== 1'b1 && guard < 100000) begin
                @(negedge ap_clk); guard = guard + 1;
            end
            @(negedge ap_clk);
            reset = do_reset; request_valid = do_request; uopc = op;
            lhs = a; rhs = b; rob_idx = ridx; allocation_id = alloc;
            pdst = dest; branch_mask = brmask; completion_ready = complete;
            branch_kill = do_kill; kill_mask = kmask; stale_owner = do_stale;
            ap_start = 1;
            @(posedge ap_clk);
            @(negedge ap_clk); ap_start = 0;
            while (ap_idle === 1'b1 && guard < 100000) begin
                @(negedge ap_clk); guard = guard + 1;
            end
            while (ap_done !== 1'b1 && guard < 100000) begin
                if (request_accepted) txn_accepted = 1;
                if (writeback_valid) begin txn_writeback = 1; txn_result = result; end
                if (rob_complete) txn_rob_complete = 1;
                if (prf_writes != 0) txn_prf_writes = prf_writes;
                if (wakeups != 0) txn_wakeups = wakeups;
                if (bypasses != 0) txn_bypasses = bypasses;
                @(negedge ap_clk); guard = guard + 1;
            end
            if (request_accepted) txn_accepted = 1;
            if (writeback_valid) begin txn_writeback = 1; txn_result = result; end
            if (rob_complete) txn_rob_complete = 1;
            if (prf_writes != 0) txn_prf_writes = prf_writes;
            if (wakeups != 0) txn_wakeups = wakeups;
            if (bypasses != 0) txn_bypasses = bypasses;
            ap_start = 0;
            if (guard == 100000) fail("ap_ctrl_timeout", "HLS transaction did not complete");
            @(negedge ap_clk);
        end
    endtask

    task clean_reset;
        begin
            cycle(1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
            if (token_valid || divider_busy || response_pending || int_result_valid)
                fail("clean_reset", "persistent execution state did not clear");
        end
    endtask

    task run_op;
        input [8*64-1:0] name;
        input [7:0] op;
        input [63:0] a, b, expected;
        integer guard, ridx, alloc;
        begin
            clean_reset();
            ridx = next_id % 31; alloc = next_id; next_id = next_id + 1;
            cycle(0, 1, op, a, b, ridx, alloc, ridx + 10, 0, 1, 0, 0, 0);
            if (!txn_accepted) fail(name, "request was not accepted");
            guard = 0;
            while (!txn_writeback && guard < 70) begin
                cycle(0, 0, 0, 0, 0, ridx, alloc, ridx + 10, 0, 1, 0, 0, 0);
                guard = guard + 1;
            end
            if (!txn_writeback || !txn_rob_complete) fail(name, "missing writeback or ROB complete");
            if (txn_result !== expected) fail(name, "arithmetic result mismatch");
            if (txn_prf_writes != 1 || txn_wakeups != 1 || txn_bypasses != 1)
                fail(name, "publication count mismatch");
            if (!lane2_inactive) fail(name, "lane2 became active");
            pass(name, "exact arithmetic result and one writeback/wakeup/bypass/ROB complete");
        end
    endtask

    task start_long_div;
        input [7:0] brmask;
        begin
            clean_reset();
            cycle(0, 1, 22, 64'hffffffffffffffff, 19, 1, 500, 10,
                  brmask, 0, 0, 0, 0);
            if (!txn_accepted || !token_valid || !divider_busy)
                fail("start_long_div", "normal divider request did not become busy");
        end
    endtask

    task overlap_mul;
        input [8*64-1:0] name;
        input [7:0] op;
        input [63:0] expected;
        begin
            start_long_div(0);
            cycle(0, 1, op, 64'hfffffffffffffffe, 3, 2, 600 + op,
                  11, 0, 1, 0, 0, 0);
            if (!txn_accepted || !txn_writeback || txn_result !== expected)
                fail(name, "multiply did not complete while divider busy");
            if (!token_valid || !divider_busy) fail(name, "divider token was disturbed");
            pass(name, "multiply completed through INT lane while divider remained busy");
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        ap_rst = 0;
        run_op("MUL", 16, 7, 9, 63);
        run_op("MULH", 17, 64'hfffffffffffffffe, 3, 64'hffffffffffffffff);
        run_op("MULHSU", 18, 64'hfffffffffffffffe, 3, 64'hffffffffffffffff);
        run_op("MULHU", 19, 64'hffffffffffffffff, 2, 1);
        run_op("MULW", 20, 64'hffffffff, 2, 64'hfffffffffffffffe);
        run_op("DIV", 21, -100, 7, -14);
        run_op("DIVU", 22, 64'hffffffffffffffff, 16, 64'h0fffffffffffffff);
        run_op("REM", 23, -100, 7, -2);
        run_op("REMU", 24, 64'hffffffffffffffff, 16, 15);
        run_op("DIVW", 25, 64'hffffff9c, 7, -14);
        run_op("DIVUW", 26, 64'hffffffff, 16, 64'h000000000fffffff);
        run_op("REMW", 27, 64'hffffff9c, 7, -2);
        run_op("REMUW", 28, 64'hffffffff, 16, 15);
        overlap_mul("divider_busy_MUL", 16, 64'hfffffffffffffffa);
        overlap_mul("divider_busy_MULH", 17, 64'hffffffffffffffff);
        overlap_mul("divider_busy_MULHSU", 18, 64'hffffffffffffffff);
        overlap_mul("divider_busy_MULHU", 19, 2);
        overlap_mul("divider_busy_MULW", 20, 64'hfffffffffffffffa);
        run_op("DIV_by_zero", 21, 123, 0, 64'hffffffffffffffff);
        run_op("REM_by_zero", 23, 123, 0, 123);
        run_op("DIV_overflow", 21, 64'h8000000000000000, -1, 64'h8000000000000000);
        run_op("REM_overflow", 23, 64'h8000000000000000, -1, 0);
        run_op("DIVW_overflow", 25, 64'h80000000, 64'hffffffff, 64'hffffffff80000000);
        run_op("REMUW_signext", 28, 64'h80000001, 0, 64'hffffffff80000001);

        start_long_div(1);
        cycle(0, 0, 0, 0, 0, 1, 500, 10, 0, 0, 1, 1, 0);
        if (token_valid || divider_busy || response_pending) fail("branch_kill", "killed divider remained active");
        pass("branch_kill", "active divider was killed without publication");

        start_long_div(0);
        clean_reset();
        pass("reset_active_divide", "reset cleared active divider state");

        start_long_div(0);
        repeat (64) cycle(0, 0, 0, 0, 0, 1, 500, 10, 0, 0, 0, 0, 0);
        clean_reset();
        pass("reset_pending_result", "reset cleared terminal or pending divider result");

        start_long_div(0);
        cycle(0, 0, 0, 0, 0, 1, 500, 10, 0, 1, 0, 0, 1);
        if (token_valid || divider_busy || response_pending || txn_writeback)
            fail("stale_allocation", "stale owner produced a side effect");
        pass("stale_allocation", "allocation mismatch canceled divider without side effect");

        clean_reset();
        cycle(0, 1, 16, 9, 9, 3, 700, 0, 0, 1, 0, 0, 0);
        if (!txn_accepted || !txn_rob_complete || txn_writeback || txn_prf_writes != 0)
            fail("rd_x0", "x0 operation publication policy mismatch");
        pass("rd_x0", "x0 destination completed without PRF publication");

        if (!lane2_inactive) fail("lane2_inactive", "unsupported lane2 was active");
        pass("lane2_inactive", "all RV64M requests remained on the INT lane");

        if (pass_count != 30) $fatal(1, "expected 30 cases, got %0d", pass_count);
        $display("M3C_RV64M_RTL_PASS cases=%0d", pass_count);
        $finish;
    end
endmodule
