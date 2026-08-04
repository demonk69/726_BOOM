`timescale 1ns/1ps

module w4_core_step_retention_tb;
  logic ap_clk = 0;
  logic ap_rst = 1;
  logic ap_start = 0;
  logic [7:0] seed;
  logic [7:0] phase;
  logic [7:0] requested_seed;
  logic [63:0] expected;
  wire ap_done;
  wire [63:0] observable;
  wire observable_ap_vld;

  always #5 ap_clk = ~ap_clk;

  synth_w4_core_step_retention_top dut (
    .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
    .seed(seed), .phase(phase), .observable(observable),
    .observable_ap_vld(observable_ap_vld), .ap_done(ap_done)
  );

  initial begin
    if (!$value$plusargs("SEED=%d", requested_seed)) $fatal(1, "missing SEED");
    if (!$value$plusargs("EXPECT=%h", expected)) $fatal(1, "missing EXPECT");
    seed = requested_seed;
    phase = 0;
    repeat (4) @(posedge ap_clk);
    ap_rst = 0;
    fork
      begin
        @(posedge ap_clk); ap_start = 1;
        @(posedge ap_clk); ap_start = 0;
        wait (ap_done && observable_ap_vld);
        if (observable !== 64'h000000000000077f)
          $fatal(1, "W4_CORE_STEP_RETENTION_STAGE0_RTL_FAIL seed=%0d expected=%016h observed=%016h",
                 requested_seed, 64'h77f, observable);
        $display("W4_CORE_STEP_RETENTION_STAGE0_RTL_PASS seed=%0d expected=%016h observed=%016h",
                 requested_seed, 64'h77f, observable);
        @(posedge ap_clk); phase = 1; ap_start = 1;
        @(posedge ap_clk); ap_start = 0;
        wait (ap_done && observable_ap_vld);
        if (observable !== expected)
          $fatal(1, "W4_CORE_STEP_RETENTION_RTL_FAIL seed=%0d expected=%016h observed=%016h",
                 requested_seed, expected, observable);
        $display("W4_CORE_STEP_RETENTION_RTL_PASS seed=%0d expected=%016h observed=%016h",
                 requested_seed, expected, observable);
        $finish;
      end
      begin
        repeat (500000) @(posedge ap_clk);
        $fatal(1, "W4_CORE_STEP_RETENTION_RTL_TIMEOUT seed=%0d", requested_seed);
      end
    join_any
  end
endmodule
