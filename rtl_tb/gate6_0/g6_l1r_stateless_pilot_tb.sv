`timescale 1ns/1ps

module g6_l1r_stateless_pilot_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg [31:0] seed = 32'h5a;
    wire ap_done, ap_idle, ap_ready;
    wire [63:0] obs0, obs1, obs2, obs3, obs4, obs5;
    integer cycles;

`ifdef PILOT_OLDER_STORE
    `DUT_TOP dut(
        .ap_start(ap_start), .ap_done(ap_done), .ap_idle(ap_idle),
        .ap_ready(ap_ready), .seed(seed), .obs0(obs0), .obs1(obs1),
        .obs2(obs2), .obs3(obs3), .obs4(obs4), .obs5(obs5));
`else
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .seed(seed), .obs0(obs0), .obs1(obs1), .obs2(obs2),
        .obs3(obs3), .obs4(obs4), .obs5(obs5));
`endif

    always #5 ap_clk = ~ap_clk;

    initial begin
        repeat (5) @(posedge ap_clk);
        ap_rst = 0;
        repeat (2) @(posedge ap_clk);
        @(negedge ap_clk);
        ap_start = 1;
        @(posedge ap_clk);
        @(negedge ap_clk);
        ap_start = 0;
        cycles = 0;
        while (!ap_done && cycles < 2000000) begin
            @(posedge ap_clk);
            cycles = cycles + 1;
        end
        if (!ap_done) $fatal(1, "G6_L1R_STATELESS_PILOT_TIMEOUT image=%0d", `PILOT_IMAGE);
`ifdef PILOT_LQ_REUSE
        if (obs0 !== 1 || obs1 !== 1 || obs2 !== 1 || obs3 !== 0 ||
            obs4 !== 1 || obs5 !== 64'h0000000000000101)
            $fatal(1, "LQ_REUSE_FAIL %h %h %h %h %h %h", obs0, obs1, obs2, obs3, obs4, obs5);
`elsif PILOT_STALE_RESPONSE
        if (obs0 !== 1 || obs1 !== 1 || obs2 !== 0 || obs3 !== 1 ||
            obs4 !== 64'h0000000000000101 || obs5 !== 64'h2300005b)
            $fatal(1, "STALE_RESPONSE_FAIL %h %h %h %h %h %h", obs0, obs1, obs2, obs3, obs4, obs5);
`elsif PILOT_BRANCH_SQUASH
        if (obs0 !== 1 || obs1 !== 0 || obs2 !== 0 || obs3 !== 0 ||
            obs4 !== 64'h0000000000000403 || obs5 !== 0)
            $fatal(1, "BRANCH_SQUASH_FAIL %h %h %h %h %h %h", obs0, obs1, obs2, obs3, obs4, obs5);
`elsif PILOT_SQ_IDENTITY
        if (obs0 !== 1 || obs1 !== 64'hffff || obs2 !== 1 ||
            obs3 !== 64'h0000000000000102 || obs4 !== 1 || obs5 !== 0)
            $fatal(1, "SQ_IDENTITY_FAIL %h %h %h %h %h %h", obs0, obs1, obs2, obs3, obs4, obs5);
`elsif PILOT_OLDER_STORE
        if (obs0 !== 1 || obs1 !== 0 || obs2 !== 0 || obs3 !== 1 ||
            obs4 !== 0 || obs5 !== 64'h000000005600005a)
            $fatal(1, "OLDER_STORE_BLOCK_FAIL %h %h %h %h %h %h", obs0, obs1, obs2, obs3, obs4, obs5);
`else
        $fatal(1, "UNKNOWN_PILOT_IMAGE");
`endif
        $display("G6_L1R_STATELESS_PILOT_PASS image=%0d cycles=%0d", `PILOT_IMAGE, cycles);
        $finish;
    end
endmodule
