`timescale 1ns/1ps

module m2b_execute_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg [7:0] seed_uopc = 0;
    reg [63:0] seed_rs1 = 0;
    reg [63:0] seed_rs2 = 0;
    wire ap_done;
    wire ap_idle;
    wire ap_ready;
    wire [63:0] observable;
    wire observable_ap_vld;

    synth_execute_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .seed_uopc(seed_uopc), .seed_rs1(seed_rs1), .seed_rs2(seed_rs2),
        .observable(observable), .observable_ap_vld(observable_ap_vld),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready)
    );

    always #5 ap_clk = ~ap_clk;

    task run_case(input [7:0] uopc, input [63:0] lhs,
                  input [63:0] rhs, input [63:0] expected);
        begin
            seed_uopc <= uopc;
            seed_rs1 <= lhs;
            seed_rs2 <= rhs;
            @(posedge ap_clk);
            ap_start <= 1;
            @(posedge ap_clk);
            ap_start <= 0;
            wait (ap_done && observable_ap_vld);
            if (observable !== expected)
                $fatal(1, "M2B_RTL_FAIL uopc=%0d expected=%016h observed=%016h",
                       uopc, expected, observable);
            $display("M2B_RTL_PASS uopc=%0d expected=%016h observed=%016h",
                     uopc, expected, observable);
            @(posedge ap_clk);
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        ap_rst <= 0;
        run_case(16, 64'hffff_ffff_ffff_fffd, 7, 64'hffff_ffff_ffff_ffeb);
        run_case(17, 64'hffff_ffff_ffff_fffd, 7, 64'hffff_ffff_ffff_ffff);
        run_case(18, 64'hffff_ffff_ffff_fffd, 7, 64'hffff_ffff_ffff_ffff);
        run_case(19, 64'hffff_ffff_ffff_fffd, 7, 6);
        run_case(20, 64'hffff_ffff_ffff_fffd, 7, 64'hffff_ffff_ffff_ffeb);
        run_case(16, 64'h1234_5678_9abc_def0, 64'hfedc_ba98_7654_3210,
                 64'h236d_88fe_5618_cf00);
        run_case(17, 64'h8000_0000_0000_0000, 64'hffff_ffff_ffff_ffff, 0);
        run_case(18, 64'h8000_0000_0000_0000, 64'hffff_ffff_ffff_ffff,
                 64'h8000_0000_0000_0000);
        run_case(19, 64'hffff_ffff_ffff_ffff, 64'hffff_ffff_ffff_ffff,
                 64'hffff_ffff_ffff_fffe);
        run_case(20, 64'h0000_0000_4000_0000, 2, 64'hffff_ffff_8000_0000);
        $display("M2B_FOCUSED_RTL_PASS cases=10");
        $finish;
    end

    initial begin
        repeat (200000) @(posedge ap_clk);
        $fatal(1, "M2B_RTL_TIMEOUT");
    end
endmodule
