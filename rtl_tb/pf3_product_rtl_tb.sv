`timescale 1ns/1ps
module pf3_product_rtl_tb;
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;
    reg clk, rst_n;
    reg [7:0] scenario_code;
    integer cycles, max_cycles, expect_trap, reset_midstream;
    integer allocations, reclaims, redirects, wraps, reuses, generation_retries;
    integer max_occupancy, reset_events, commits_before_reset;
    reg first_fetch_seen, first_fetch_error, did_reset, saw_exception;
    reg previous_redirect, previous_retire;
    reg [4:0] previous_head;
    reg [7:0] previous_tail, previous_count;
    reg [31:0] previous_generation;
    string program_name;
    wire tohost_seen, tohost_commit_seen, protocol_error;
    wire [63:0] tohost_value;
    wire [31:0] commit_count;
    wire [127:0] observed_imem_req;
    wire observed_imem_transfer;
    wire [4:0] ftq_head;
    wire [7:0] ftq_tail, ftq_count;
    wire [31:0] ftq_next_generation;
    wire ftq_retire_pending, ftq_redirect_pending, exception_valid;

    pf3_product_rtl_harness harness (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .tohost_seen(tohost_seen), .tohost_value(tohost_value),
        .tohost_commit_seen(tohost_commit_seen), .protocol_error(protocol_error),
        .commit_count(commit_count), .observed_imem_req(observed_imem_req),
        .observed_imem_transfer(observed_imem_transfer), .ftq_head(ftq_head),
        .ftq_tail(ftq_tail), .ftq_count(ftq_count),
        .ftq_next_generation(ftq_next_generation), .ftq_retire_pending(ftq_retire_pending),
        .ftq_redirect_pending(ftq_redirect_pending), .exception_valid(exception_valid));

    initial begin clk = 1'b0; forever #5 clk = ~clk; end

    always @(posedge clk) begin
        if (rst_n) begin
            if (observed_imem_transfer && !first_fetch_seen) begin
                first_fetch_seen <= 1'b1;
                if (observed_imem_req[63:0] != RESET_VECTOR) first_fetch_error <= 1'b1;
            end
            if (ftq_next_generation != previous_generation) begin
                allocations = allocations + 1;
                if (allocations > 32) reuses = reuses + 1;
            end
            if (ftq_tail < previous_tail) wraps = wraps + 1;
            if (ftq_head != previous_head) reclaims = reclaims + 1;
            if (ftq_redirect_pending && !previous_redirect) redirects = redirects + 1;
            if (ftq_redirect_pending && ftq_retire_pending && previous_retire)
                generation_retries = generation_retries + 1;
            if (ftq_count > max_occupancy) max_occupancy = ftq_count;
            if (exception_valid) saw_exception <= 1'b1;
            previous_head <= ftq_head;
            previous_tail <= ftq_tail;
            previous_count <= ftq_count;
            previous_generation <= ftq_next_generation;
            previous_redirect <= ftq_redirect_pending;
            previous_retire <= ftq_retire_pending;
        end
    end

    task automatic pulse_reset;
        begin
            @(negedge clk); rst_n = 1'b0; reset_events = reset_events + 1;
            repeat (4) @(negedge clk);
            rst_n = 1'b1; did_reset = 1'b1;
        end
    endtask

    initial begin
        if (!$value$plusargs("PROGRAM_NAME=%s", program_name)) program_name = "pf3_straight_commit";
        if (!$value$plusargs("EXPECT_TRAP=%d", expect_trap)) expect_trap = 0;
        if (!$value$plusargs("RESET_MIDSTREAM=%d", reset_midstream)) reset_midstream = 0;
        if (!$value$plusargs("MAX_CYCLES=%d", max_cycles)) max_cycles = 1500000;
        scenario_code = 0; rst_n = 0; cycles = 0;
        allocations = 0; reclaims = 0; redirects = 0; wraps = 0; reuses = 0;
        generation_retries = 0; max_occupancy = 0; reset_events = 0;
        first_fetch_seen = 0; first_fetch_error = 0; did_reset = 0; saw_exception = 0;
        previous_head = 0; previous_tail = 0; previous_count = 0; previous_generation = 1;
        previous_redirect = 0; previous_retire = 0; commits_before_reset = 0;
        repeat (5) @(negedge clk); rst_n = 1;
        if (reset_midstream) begin
            fork begin
                wait (commit_count >= 20);
                commits_before_reset = commit_count;
                pulse_reset();
            end join_none
        end
        while (cycles < max_cycles &&
               !(expect_trap ? (saw_exception && commit_count >= 5) :
                 (tohost_seen && tohost_commit_seen && (!reset_midstream || did_reset)))) begin
            @(posedge clk); cycles = cycles + 1;
        end
        @(posedge clk);
        if (cycles >= max_cycles || protocol_error || first_fetch_error || !first_fetch_seen)
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s timeout/protocol/fetch", program_name);
        if (expect_trap && (!saw_exception || tohost_seen || tohost_commit_seen))
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s exception contract", program_name);
        if (!expect_trap && (tohost_value != 1 || !tohost_commit_seen))
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s completion contract", program_name);
        $display("PF3_RTL_EVENT program=%s alloc=%0d reclaim=%0d redirect=%0d wrap=%0d reuse=%0d generation_retry=%0d reset=%0d max_occupancy=%0d commits=%0d cycles=%0d",
                 program_name, allocations, reclaims, redirects, wraps, reuses,
                 generation_retries, reset_events, max_occupancy, commit_count, cycles);
        if (allocations == 0 || reclaims == 0)
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s missing FTQ allocation/reclaim", program_name);
        if ((program_name == "pf3_jal_mask" || program_name == "pf3_conditional_shadow" ||
             program_name == "pf3_branch_squash" || program_name == "pf3_generation_reuse" ||
             program_name == "pf3_mixed_control") && redirects == 0)
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s missing redirect", program_name);
        if ((program_name == "pf3_ftq_wrap" || program_name == "pf3_generation_reuse" ||
             program_name == "pf3_long_stream" || program_name == "pf3_reset_midstream") &&
            (wraps == 0 || reuses == 0))
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s missing wrap/reuse", program_name);
        if (reset_midstream && (reset_events != 1 || commit_count <= commits_before_reset))
            $fatal(1, "PF3_PRODUCT_RTL_FAIL program=%s reset contract", program_name);
        harness.trace_monitor.finish_trace("pass");
        $display("PF3_PRODUCT_RTL_PASS program=%s", program_name);
        $finish;
    end
endmodule
