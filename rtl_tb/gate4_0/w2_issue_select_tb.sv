`timescale 1ns/1ps

module w2_issue_select_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg [31:0] seed_inst;
    reg [63:0] seed_pc = 64'h10040;
    wire ap_done;
    wire ap_idle;
    wire ap_ready;
    wire [63:0] observable;
    wire observable_ap_vld;
    integer expected_count, expected_generated, expected_accepted, expected_retained;
    integer expected_mem_valid, expected_mem_accepted, expected_int_valid, expected_int_accepted;
    integer expected_survivor_rob, expected_issued_rob;

    synth_issue_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .seed_inst(seed_inst), .seed_pc(seed_pc), .ap_done(ap_done),
        .ap_idle(ap_idle), .ap_ready(ap_ready), .observable(observable),
        .observable_ap_vld(observable_ap_vld)
    );

    always #5 ap_clk = ~ap_clk;

    initial begin
        if (!$value$plusargs("SEED=%d", seed_inst)) $fatal(1, "missing SEED");
        if (!$value$plusargs("COUNT=%d", expected_count)) $fatal(1, "missing COUNT");
        if (!$value$plusargs("GENERATED=%d", expected_generated)) $fatal(1, "missing GENERATED");
        if (!$value$plusargs("ACCEPTED=%d", expected_accepted)) $fatal(1, "missing ACCEPTED");
        if (!$value$plusargs("RETAINED=%d", expected_retained)) $fatal(1, "missing RETAINED");
        if (!$value$plusargs("MEM_VALID=%d", expected_mem_valid)) $fatal(1, "missing MEM_VALID");
        if (!$value$plusargs("MEM_ACCEPTED=%d", expected_mem_accepted)) $fatal(1, "missing MEM_ACCEPTED");
        if (!$value$plusargs("INT_VALID=%d", expected_int_valid)) $fatal(1, "missing INT_VALID");
        if (!$value$plusargs("INT_ACCEPTED=%d", expected_int_accepted)) $fatal(1, "missing INT_ACCEPTED");
        if (!$value$plusargs("SURVIVOR_ROB=%d", expected_survivor_rob)) $fatal(1, "missing SURVIVOR_ROB");
        if (!$value$plusargs("ISSUED_ROB=%d", expected_issued_rob)) $fatal(1, "missing ISSUED_ROB");
        repeat (4) @(posedge ap_clk);
        ap_rst <= 0;
        @(posedge ap_clk);
        ap_start <= 1;
        @(posedge ap_clk);
        ap_start <= 0;
        wait (ap_done && observable_ap_vld);
        if (observable[7:0] !== expected_count[7:0] ||
            observable[15:8] !== expected_generated[7:0] ||
            observable[23:16] !== expected_accepted[7:0] ||
            observable[31:24] !== expected_retained[7:0] ||
            observable[39:32] !== 0 || observable[40] !== expected_mem_valid[0] ||
            observable[41] !== expected_mem_accepted[0] || observable[42] !== expected_int_valid[0] ||
            observable[43] !== expected_int_accepted[0] || observable[44] !== 0 ||
            observable[55:48] !== expected_survivor_rob[7:0] ||
            observable[63:56] !== expected_issued_rob[7:0]) begin
            $fatal(1, "W2_RTL_MISMATCH observable=%h", observable);
        end
        $display("W2_RTL_PASS observable=%h", observable);
        $finish;
    end
endmodule
