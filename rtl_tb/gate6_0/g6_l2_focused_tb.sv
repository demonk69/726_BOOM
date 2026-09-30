`timescale 1ns/1ps
module g6_l2_focused_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    wire ap_done;
    wire ap_idle;
    wire ap_ready;
    reg [63:0] load_meta, load_address;
    reg [31:0] load_allocation;
    reg [63:0] store0_rob, store0_sq, store0_address, store0_data;
    reg [7:0] store0_mask;
    reg [63:0] store1_rob, store1_sq, store1_address, store1_data;
    reg [7:0] store1_mask;
    reg [63:0] store2_rob, store2_sq, store2_address, store2_data;
    reg [7:0] store2_mask;
    wire [63:0] decision, selected_sq, value, coverage;
    reg [63:0] expected_decision, expected_selected, expected_value, expected_coverage;
    integer case_id, cycles;

    always #5 ap_clk = ~ap_clk;

    g6_l2_focused_top dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .load_meta(load_meta), .load_allocation(load_allocation),
        .load_address(load_address),
        .store0_rob(store0_rob), .store0_sq(store0_sq),
        .store0_address(store0_address), .store0_data(store0_data),
        .store0_mask(store0_mask),
        .store1_rob(store1_rob), .store1_sq(store1_sq),
        .store1_address(store1_address), .store1_data(store1_data),
        .store1_mask(store1_mask),
        .store2_rob(store2_rob), .store2_sq(store2_sq),
        .store2_address(store2_address), .store2_data(store2_data),
        .store2_mask(store2_mask),
        .decision(decision), .selected_sq(selected_sq), .value_r(value),
        .coverage(coverage));

    initial begin
        if (!$value$plusargs("CASE_ID=%d", case_id)) $fatal(1, "missing CASE_ID");
        if (!$value$plusargs("LOAD_META=%h", load_meta)) $fatal(1, "missing LOAD_META");
        if (!$value$plusargs("LOAD_ALLOC=%h", load_allocation)) $fatal(1, "missing LOAD_ALLOC");
        if (!$value$plusargs("LOAD_ADDR=%h", load_address)) $fatal(1, "missing LOAD_ADDR");
        if (!$value$plusargs("S0_ROB=%h", store0_rob)) $fatal(1, "missing S0_ROB");
        if (!$value$plusargs("S0_SQ=%h", store0_sq)) $fatal(1, "missing S0_SQ");
        if (!$value$plusargs("S0_ADDR=%h", store0_address)) $fatal(1, "missing S0_ADDR");
        if (!$value$plusargs("S0_DATA=%h", store0_data)) $fatal(1, "missing S0_DATA");
        if (!$value$plusargs("S0_MASK=%h", store0_mask)) $fatal(1, "missing S0_MASK");
        if (!$value$plusargs("S1_ROB=%h", store1_rob)) $fatal(1, "missing S1_ROB");
        if (!$value$plusargs("S1_SQ=%h", store1_sq)) $fatal(1, "missing S1_SQ");
        if (!$value$plusargs("S1_ADDR=%h", store1_address)) $fatal(1, "missing S1_ADDR");
        if (!$value$plusargs("S1_DATA=%h", store1_data)) $fatal(1, "missing S1_DATA");
        if (!$value$plusargs("S1_MASK=%h", store1_mask)) $fatal(1, "missing S1_MASK");
        if (!$value$plusargs("S2_ROB=%h", store2_rob)) $fatal(1, "missing S2_ROB");
        if (!$value$plusargs("S2_SQ=%h", store2_sq)) $fatal(1, "missing S2_SQ");
        if (!$value$plusargs("S2_ADDR=%h", store2_address)) $fatal(1, "missing S2_ADDR");
        if (!$value$plusargs("S2_DATA=%h", store2_data)) $fatal(1, "missing S2_DATA");
        if (!$value$plusargs("S2_MASK=%h", store2_mask)) $fatal(1, "missing S2_MASK");
        if (!$value$plusargs("EXP_DECISION=%h", expected_decision)) $fatal(1, "missing EXP_DECISION");
        if (!$value$plusargs("EXP_SELECTED=%h", expected_selected)) $fatal(1, "missing EXP_SELECTED");
        if (!$value$plusargs("EXP_VALUE=%h", expected_value)) $fatal(1, "missing EXP_VALUE");
        if (!$value$plusargs("EXP_COVERAGE=%h", expected_coverage)) $fatal(1, "missing EXP_COVERAGE");
        repeat (5) @(posedge ap_clk);
        ap_rst = 0;
        repeat (2) @(posedge ap_clk);
        @(negedge ap_clk); ap_start = 1;
        @(posedge ap_clk);
        @(negedge ap_clk); ap_start = 0;
        cycles = 0;
        while (!ap_done && cycles < 2000000) begin
            @(posedge ap_clk);
            cycles = cycles + 1;
        end
        if (!ap_done) $fatal(1, "G6_L2_FOCUSED_TIMEOUT id=%0d", case_id);
        if (decision !== expected_decision ||
            selected_sq !== expected_selected || value !== expected_value ||
            coverage !== expected_coverage)
            $fatal(1, "G6_L2_FOCUSED_FAIL id=%0d got=%h:%h:%h:%h expected=%h:%h:%h:%h",
                case_id, decision, selected_sq, value, coverage,
                expected_decision, expected_selected, expected_value,
                expected_coverage);
        $display("G6_L2_FOCUSED_CASE_PASS id=%0d cycles=%0d", case_id, cycles);
        $finish;
    end
endmodule
