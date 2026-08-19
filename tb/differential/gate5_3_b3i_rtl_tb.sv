`timescale 1ns/1ps
module gate5_3_b3i_rtl_tb;
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;
    reg clk;
    reg rst_n;
    reg [7:0] scenario_code;
    integer cycles;
    integer max_cycles;
    integer expect_trap;
    reg first_fetch_seen;
    reg first_fetch_error;
    string program_name;
    string scenario;
    wire tohost_seen, tohost_commit_seen, io_trap, protocol_error;
    wire [63:0] tohost_value;
    wire [31:0] commit_count;
    wire [127:0] observed_imem_req;
    wire observed_imem_transfer;

    gate5_3_b3i_rtl_harness harness (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .tohost_seen(tohost_seen), .tohost_value(tohost_value),
        .tohost_commit_seen(tohost_commit_seen), .io_trap(io_trap),
        .protocol_error(protocol_error), .commit_count(commit_count),
        .observed_imem_req(observed_imem_req), .observed_imem_transfer(observed_imem_transfer));

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    always @(posedge clk) begin
        if (rst_n && observed_imem_transfer && !first_fetch_seen) begin
            first_fetch_seen <= 1'b1;
            if (observed_imem_req[63:0] != RESET_VECTOR) first_fetch_error <= 1'b1;
        end
    end

    initial begin
        if (!$value$plusargs("PROGRAM_NAME=%s", program_name)) program_name = "packet_two_rvc";
        if (!$value$plusargs("SCENARIO=%s", scenario)) scenario = "R0_POWER_ON_RESET";
        if (!$value$plusargs("EXPECT_TRAP=%d", expect_trap)) expect_trap = 0;
        if (!$value$plusargs("MAX_CYCLES=%d", max_cycles)) max_cycles = 1000000;
        scenario_code = scenario == "I3_IMEM_RESPONSE_DELAY_4" ? 8'd23 : 8'd0;
        rst_n = 1'b0;
        cycles = 0;
        first_fetch_seen = 1'b0;
        first_fetch_error = 1'b0;
        repeat (5) @(negedge clk);
        rst_n = 1'b1;
        while (cycles < max_cycles &&
               !(expect_trap ? (io_trap === 1'b1) :
                 (tohost_seen === 1'b1 && tohost_commit_seen === 1'b1))) begin
            @(posedge clk);
            cycles = cycles + 1;
        end
        @(posedge clk);
        if (cycles >= max_cycles || protocol_error || first_fetch_error || !first_fetch_seen)
            $fatal(1, "GATE5_3_B3I_RTL_FAIL program=%s timeout/protocol/fetch", program_name);
        if (expect_trap) begin
            if (io_trap !== 1'b1 || tohost_seen === 1'b1 || tohost_commit_seen === 1'b1)
                $fatal(1, "GATE5_3_B3I_RTL_FAIL program=%s trap contract", program_name);
        end else if (io_trap || tohost_value != 64'd1) begin
            $fatal(1, "GATE5_3_B3I_RTL_FAIL program=%s normal contract", program_name);
        end
        harness.trace_monitor.finish_trace("pass");
        $display("GATE5_3_B3I_RTL_PASS program=%s cycles=%0d commits=%0d trap=%0d",
                 program_name, cycles, commit_count, expect_trap);
        $finish;
    end
endmodule
