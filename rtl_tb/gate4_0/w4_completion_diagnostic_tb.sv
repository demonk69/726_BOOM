`timescale 1ns/1ps

module w4_completion_diagnostic_tb;
  logic ap_clk = 0;
  logic ap_rst = 1;
  logic [7:0] scenario;
  logic [7:0] requested;
  logic [63:0] expected;
  wire [63:0] observable;
  wire observable_ap_vld;

  always #5 ap_clk = ~ap_clk;

  synth_w4d_oracle_top dut (
    .ap_clk(ap_clk), .ap_rst(ap_rst), .scenario(scenario),
    .observable(observable), .observable_ap_vld(observable_ap_vld)
  );

  initial begin
    if (!$value$plusargs("SCENARIO=%d", requested)) $fatal(1, "missing SCENARIO");
    if (!$value$plusargs("EXPECT=%h", expected)) $fatal(1, "missing EXPECT");
    scenario = requested;
    repeat (4) @(posedge ap_clk);
    ap_rst = 0;
    fork
      begin
        do @(posedge ap_clk); while (!observable_ap_vld);
        if (observable !== expected)
          $fatal(1, "W4_RTL_FAIL scenario=%0d expected=%016h observed=%016h",
                 requested, expected, observable);
        $display("W4_RTL_PASS scenario=%0d expected=%016h observed=%016h",
                 requested, expected, observable);
        $finish;
      end
      begin
        repeat (200000) @(posedge ap_clk);
        $fatal(1, "W4_RTL_TIMEOUT scenario=%0d", requested);
      end
    join_any
  end
endmodule
