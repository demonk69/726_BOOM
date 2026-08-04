`timescale 1ns/1ps

// Prepared for focused post-W4D RTL validation. Scenario encoding and packed
// expectations are shared with w4d_rtl_oracle_tests.cpp.
module w4d_writeback_oracle_tb;
  logic ap_clk = 0;
  logic ap_rst = 1;
  logic [7:0] scenario;
  wire [63:0] observable;
  wire observable_ap_vld;

  always #5 ap_clk = ~ap_clk;

  synth_w4d_oracle_top dut (
    .ap_clk(ap_clk), .ap_rst(ap_rst),
    .scenario(scenario), .observable(observable),
    .observable_ap_vld(observable_ap_vld)
  );

  task automatic check(input [7:0] s, input [63:0] mask,
                       input [63:0] expected);
    begin
      scenario = s;
      do @(posedge ap_clk); while (!observable_ap_vld);
      if ((observable & mask) !== expected)
        $fatal(1, "scenario %0d observed=%h mask=%h expected=%h",
               s, observable, mask, expected);
    end
  endtask

  initial begin
    scenario = 0;
    repeat (4) @(posedge ap_clk);
    ap_rst = 0;
    check(0,    64'h000000000003000f, 64'h0000000000030002);
    check(1,    64'h000000000000030f, 64'h0000000000000100);
    check(2,    64'h000000000000240f, 64'h0000000000002400);
    check(8'h82,64'h000000000004000f, 64'h0000000000040001);
    check(3,    64'h000000000000080f, 64'h0000000000000802);
    check(8'h83,64'h000000000038000f, 64'h0000000000380001);
    check(4,    64'h000000000040010f, 64'h0000000000400100);
    check(5,    64'h000000000000090f, 64'h0000000000000900);
    check(6,    64'h000000000380010f, 64'h0000000001800001);
    check(7,    64'h000000003c0009ff, 64'h000000003c000900);
    $display("W4D RTL oracle: 10 passed, 0 failed");
    $finish;
  end
endmodule
