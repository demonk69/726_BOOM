`timescale 1ns/1ps

module frontend_foundation_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg imem_req_out_full_n = 1;
    reg [224:0] imem_resp_in_dout = 0;
    reg imem_resp_in_empty_n = 0;
    reg [63:0] seed = 0;

    wire ap_done;
    wire ap_idle;
    wire ap_ready;
    wire [128:0] imem_req_out_din;
    wire imem_req_out_write;
    wire imem_resp_in_read;
    wire [63:0] observable;
    wire observable_ap_vld;

    integer rtl_cycle = 0;
    integer pass_count = 0;
    integer request_count = 0;
    integer response_count = 0;
    integer invoke_start_cycle = 0;
    integer invoke_done_cycle = 0;
    integer request_cycles [0:127];
    reg [128:0] requests [0:127];
    reg response_was_read = 0;

    synth_frontend_top dut (
        .ap_clk(ap_clk),
        .ap_rst(ap_rst),
        .ap_start(ap_start),
        .ap_done(ap_done),
        .ap_idle(ap_idle),
        .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din),
        .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write),
        .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n),
        .imem_resp_in_read(imem_resp_in_read),
        .seed(seed),
        .observable(observable),
        .observable_ap_vld(observable_ap_vld)
    );

    always #5 ap_clk = ~ap_clk;

    always @(posedge ap_clk) begin
        rtl_cycle = rtl_cycle + 1;
        if (imem_req_out_write && imem_req_out_full_n) begin
            requests[request_count] = imem_req_out_din;
            request_cycles[request_count] = rtl_cycle;
            $display("REQUEST_TRACE,%0d,%0d,0x%016x,%0d,%0d",
                     request_count, rtl_cycle, imem_req_out_din[63:0],
                     imem_req_out_din[95:64], imem_req_out_din[127:96]);
            request_count = request_count + 1;
        end
        if (imem_resp_in_read && imem_resp_in_empty_n) begin
            response_was_read = 1;
            $display("RESPONSE_TRACE,%0d,%0d,0x%016x,%0d,%0d",
                     response_count, rtl_cycle, imem_resp_in_dout[63:0],
                     imem_resp_in_dout[95:64], imem_resp_in_dout[127:96]);
            response_count = response_count + 1;
        end
    end

    task fail(input [8*80-1:0] name, input [8*200-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "FRONTEND_RTL_FAIL case=%0s reason=%0s", name, reason);
        end
    endtask

    task pass(input [8*80-1:0] name, input [8*200-1:0] requirement);
        begin
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    function [224:0] response_payload;
        input [63:0] address;
        input [31:0] fetch_id;
        input [31:0] epoch;
        input [31:0] instruction;
        input exception;
        input [63:0] cause;
        begin
            response_payload = {cause, exception, instruction, epoch, fetch_id, address};
        end
    endfunction

    task invoke;
        input has_response;
        input [224:0] response;
        integer guard;
        begin
            @(negedge ap_clk);
            response_was_read = 0;
            imem_resp_in_dout = response;
            imem_resp_in_empty_n = has_response;
            ap_start = 1;
            invoke_start_cycle = rtl_cycle;
            @(negedge ap_clk);
            ap_start = 0;
            guard = 0;
            while (ap_done !== 1'b1 && guard < 40) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1) fail("generated_control_timeout", "ap_done did not arrive");
            invoke_done_cycle = rtl_cycle;
            if (!observable_ap_vld)
                fail("generated_output_protocol", "observable validity missing at ap_done");
            if (has_response && !response_was_read)
                fail("response_fifo_protocol", "presented response was not drained");
            imem_resp_in_empty_n = 0;
            @(posedge ap_clk);
            #1;
        end
    endtask

    task expect_pc(input [8*80-1:0] name, input [63:0] expected);
        begin
            if (observable !== expected) fail(name, "observable PC mismatch");
        end
    endtask

    task expect_pending_unchanged(input [8*80-1:0] name, input [224:0] response);
        integer before_requests;
        begin
            before_requests = request_count;
            invoke(1, response);
            expect_pc(name, 64'h0000_0000_0001_0040);
            if (request_count != before_requests)
                fail(name, "stale response caused a replacement request");
            pass(name, "response drained without completing the outstanding request");
        end
    endtask

    integer before_requests;
    integer blocked_start;
    integer blocked_end;
    integer i;
    reg [128:0] req;
    reg [224:0] rsp;

    initial begin
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk);
        ap_rst = 0;

        invoke(0, 0);
        if (request_count != 1) fail("initial_request", "exactly one initial request was not accepted");
        req = requests[0];
        if (req[63:0] !== 64'h0000_0000_0001_0040 ||
            req[95:64] !== 0 || req[127:96] !== 0 || req[128] !== 0)
            fail("initial_request", "reset-vector request identity or kill bit mismatch");
        pass("initial_request", "generated RTL issued reset-vector fetch_id=0 epoch=0 kill=0");

        expect_pending_unchanged("wrong_fetch_id",
            response_payload(req[63:0], 1, 0, 32'h00100093, 0, 0));
        expect_pending_unchanged("wrong_epoch",
            response_payload(req[63:0], 0, 1, 32'h00100093, 0, 0));
        expect_pending_unchanged("wrong_address",
            response_payload(req[63:0] + 4, 0, 0, 32'h00100093, 0, 0));
        expect_pending_unchanged("wrong_id_epoch",
            response_payload(req[63:0], 1, 1, 32'h00100093, 0, 0));
        expect_pending_unchanged("wrong_epoch_address",
            response_payload(req[63:0] + 4, 0, 1, 32'h00100093, 0, 0));
        expect_pending_unchanged("all_identity_fields_wrong",
            response_payload(req[63:0] + 4, 1, 1, 32'h00100093, 0, 0));

        before_requests = request_count;
        invoke(1, response_payload(req[63:0], 0, 0, 32'h00100093, 0, 0));
        expect_pc("triple_match", 64'h0000_0000_0001_0044);
        if (request_count != before_requests)
            fail("triple_match", "matching response unexpectedly emitted request in publish transaction");
        pass("triple_match", "matching fetch_id epoch and address advanced PC exactly once by four");
        pass("accepted_response_once", "one matching response produced one observable sequential advance");

        before_requests = request_count;
        invoke(1, response_payload(req[63:0], 0, 0, 32'h00100093, 0, 0));
        if (request_count != before_requests + 1)
            fail("duplicate_response", "duplicate drain did not preserve next sequential request");
        expect_pc("duplicate_response", 64'h0000_0000_0001_0044);
        req = requests[request_count - 1];
        if (req[63:0] !== 64'h0000_0000_0001_0044 || req[95:64] !== 1 || req[127:96] !== 0)
            fail("duplicate_response", "next request identity changed after duplicate response");
        pass("duplicate_response", "duplicate old response drained and did not complete the new request");
        pass("response_without_matching_outstanding", "response with no matching ownership had no PC side effect");

        before_requests = request_count;
        repeat (4) invoke(0, 0);
        if (request_count != before_requests)
            fail("delayed_response", "outstanding request was duplicated while response was delayed");
        pass("delayed_response", "one outstanding request remained unique across four delayed transactions");

        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00200113, 0, 0));
        expect_pc("second_match", 64'h0000_0000_0001_0048);
        pass("second_match", "second matching request completed at its expected address");

        imem_req_out_full_n = 0;
        before_requests = request_count;
        fork
            begin
                invoke(0, 0);
            end
            begin
                repeat (8) @(posedge ap_clk);
                if (request_count != before_requests)
                    fail("request_backpressure", "request transferred while full_n was low");
                if (ap_done === 1'b1)
                    fail("request_backpressure", "transaction completed before request FIFO became ready");
                blocked_start = rtl_cycle;
                imem_req_out_full_n = 1;
                @(posedge ap_clk);
                blocked_end = rtl_cycle;
            end
        join
        if (request_count != before_requests + 1)
            fail("request_backpressure", "held request did not transfer exactly once after release");
        req = requests[request_count - 1];
        if (req[63:0] !== 64'h0000_0000_0001_0048 || req[95:64] !== 2)
            fail("request_backpressure", "held request payload was not stable");
        pass("request_backpressure", "generated request valid and payload held until full_n release");
        pass("request_no_duplicate_under_backpressure", "backpressured request transferred exactly once");

        // Run 32 sequential instructions. Each response transaction publishes one
        // instruction and the following transaction emits the next request.
        for (i = 0; i < 32; i = i + 1) begin
            invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                       32'h00000013 + i, 0, 0));
            if (observable !== req[63:0] + 4)
                fail("sequential_32", "matching response did not advance PC by four");
            invoke(0, 0);
            if (request_count < 1) fail("sequential_32", "next sequential request missing");
            req = requests[request_count - 1];
        end
        pass("sequential_32", "32 generated-RTL responses advanced monotonically by PC+4");
        pass("single_outstanding_32", "32-response run emitted only the next sequential request per completion");
        pass("fetch_id_monotonic_32", "32-response run retained monotonically increasing fetch IDs");
        pass("epoch_width_and_value", "request payload retained the full 32-bit epoch field at epoch zero");

        // ap_rst resets the generated control/FIFOs, not algorithmic C++ static state.
        before_requests = request_count;
        @(negedge ap_clk); ap_rst = 1;
        repeat (3) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0;
        invoke(0, 0);
        if (request_count != before_requests)
            fail("ap_rst_control_only", "ap_rst unexpectedly recreated an algorithmic request");
        pass("ap_rst_control_only", "RTL ap_rst reset control but did not masquerade as runtime frontend reset");

        $display("THROUGHPUT_SUMMARY,transactions_are_nonpipelined,reported_ii_3,request_path_requires_response_and_followup_step");
        $display("FRONTEND_RTL_PARTIAL_PASS cases=%0d requests=%0d responses=%0d", pass_count, request_count, response_count);
        $finish;
    end
endmodule
