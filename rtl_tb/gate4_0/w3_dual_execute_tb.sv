`timescale 1ns/1ps

module w3_dual_execute_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg completion_start = 0;
    reg [7:0] scenario;
    reg [7:0] requested;
    reg [63:0] expected;
    wire ap_done;
    wire ap_idle;
    wire ap_ready;
    wire [63:0] observable;
    wire observable_ap_vld;
    wire completion_done;
    wire completion_idle;
    wire completion_ready;
    wire [63:0] completion_observable;
    wire completion_observable_ap_vld;
    wire dual_pending_done;
    wire dual_pending_observable_ap_vld;
    wire [63:0] dual_pending_observable;
    wire rob_wrap_done;
    wire rob_wrap_observable_ap_vld;
    wire [63:0] rob_wrap_observable;
    reg dual_pending_start = 0;
    reg rob_wrap_start = 0;
    reg [31:0] allocation_base = 0;
    reg branch_kill_start = 0;
    reg [7:0] branch_control = 8'h3f;
    wire branch_kill_done;
    wire branch_kill_observable_ap_vld;
    wire [63:0] branch_kill_observable;

    synth_w3_diagnostic_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .scenario(scenario), .observable(observable),
        .observable_ap_vld(observable_ap_vld), .ap_done(ap_done),
        .ap_idle(ap_idle), .ap_ready(ap_ready)
    );

    synth_w3_dual_pending_top dual_pending_dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst),
        .ap_start(dual_pending_start),
        .allocation_base(allocation_base),
        .observable(dual_pending_observable),
        .observable_ap_vld(dual_pending_observable_ap_vld), .ap_done(dual_pending_done)
    );

    synth_w3_rob_wrap_top rob_wrap_dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst),
        .ap_start(rob_wrap_start),
        .allocation_base(allocation_base),
        .observable(rob_wrap_observable),
        .observable_ap_vld(rob_wrap_observable_ap_vld), .ap_done(rob_wrap_done)
    );

    synth_w3_branch_kill_top branch_kill_dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(branch_kill_start),
        .allocation_base(allocation_base),
        .control(branch_control),
        .observable(branch_kill_observable),
        .observable_ap_vld(branch_kill_observable_ap_vld), .ap_done(branch_kill_done)
    );

    synth_w3_completion_diagnostic_top completion_dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(completion_start),
        .scenario(scenario), .observable(completion_observable),
        .observable_ap_vld(completion_observable_ap_vld), .ap_done(completion_done),
        .ap_idle(completion_idle), .ap_ready(completion_ready)
    );

    always #5 ap_clk = ~ap_clk;

    task run_scenario(input [7:0] selected);
        reg use_completion;
        begin
            use_completion = selected == 10;
            if (selected == 3) allocation_base <= 31;
            if (selected == 4) allocation_base <= 41;
            if (selected == 9) allocation_base <= 91;
            scenario <= selected;
            @(posedge ap_clk);
            if (selected == 3) dual_pending_start <= 1;
            else if (selected == 4) branch_kill_start <= 1;
            else if (selected == 9) rob_wrap_start <= 1;
            else if (use_completion) completion_start <= 1;
            else ap_start <= 1;
            @(posedge ap_clk);
            ap_start <= 0;
            completion_start <= 0;
            dual_pending_start <= 0;
            rob_wrap_start <= 0;
            branch_kill_start <= 0;
            if (selected == 3) wait (dual_pending_done && dual_pending_observable_ap_vld);
            else if (selected == 4) wait (branch_kill_done && branch_kill_observable_ap_vld);
            else if (selected == 9) wait (rob_wrap_done && rob_wrap_observable_ap_vld);
            else if (use_completion) wait (completion_done && completion_observable_ap_vld);
            else wait (ap_done && observable_ap_vld);
            @(posedge ap_clk);
        end
    endtask

    initial begin
        if (!$value$plusargs("SCENARIO=%d", requested)) $fatal(1, "missing SCENARIO");
        if (!$value$plusargs("EXPECT=%h", expected)) $fatal(1, "missing EXPECT");
        repeat (4) @(posedge ap_clk);
        ap_rst <= 0;
        fork
            begin
                if (requested == 7) run_scenario(107);
                if (requested == 8) run_scenario(108);
                run_scenario(requested);
                if (requested == 3 && dual_pending_observable !== expected)
                    $fatal(1, "W3_RTL_FAIL scenario=%0d expected=%016h observed=%016h",
                           scenario, expected, dual_pending_observable);
                if (requested == 9 && rob_wrap_observable !== expected)
                    $fatal(1, "W3_RTL_FAIL scenario=%0d expected=%016h observed=%016h",
                           scenario, expected, rob_wrap_observable);
                if (requested == 4 && branch_kill_observable !== expected)
                    $fatal(1, "W3_RTL_FAIL scenario=%0d expected=%016h observed=%016h",
                           scenario, expected, branch_kill_observable);
                if (requested == 10 &&
                    completion_observable !== expected)
                    $fatal(1, "W3_RTL_FAIL scenario=%0d expected=%016h observed=%016h",
                           scenario, expected, completion_observable);
                if (!(requested == 3 || requested == 4 || requested == 9 || requested == 10) &&
                    observable !== expected)
                    $fatal(1, "W3_RTL_FAIL scenario=%0d expected=%016h observed=%016h",
                           scenario, expected, observable);
                $display("W3_RTL_PASS scenario=%0d expected=%016h observed=%016h",
                         scenario, expected,
                         requested == 3 ? dual_pending_observable :
                         (requested == 9 ? rob_wrap_observable :
                         (requested == 4 ? branch_kill_observable :
                         (requested == 10 ? completion_observable : observable))));
                $finish;
            end
            begin
                repeat (200000) @(posedge ap_clk);
                $fatal(1, "W3_RTL_TIMEOUT scenario=%0d", scenario);
            end
        join_any
    end
endmodule
