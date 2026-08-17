`timescale 1ns/1ps

module fetch_buffer_integration_rtl_tb;
    reg ap_clk = 0, ap_rst = 1, ap_start = 0;
    reg imem_req_out_full_n = 1, imem_resp_in_empty_n = 0;
    reg [224:0] imem_resp_in_dout = 0;
    reg runtime_reset = 0, generic_flush = 0, decode_ready = 0;
    reg canonical_redirect_valid = 0;
    reg [63:0] canonical_redirect_target = 0;
    reg push_valid = 0, push_is_rvc = 0, push_exception = 0;
    reg [63:0] push_pc = 0, push_cause = 0;
    reg [31:0] push_instruction = 0, push_original = 0, push_fetch_id = 0;
    wire ap_done, ap_idle, ap_ready, imem_req_out_write, imem_resp_in_read;
    wire [128:0] imem_req_out_din;
    wire push_held, decode_valid, decode_is_rvc, decode_exception, full;
    wire [63:0] decode_pc, decode_cause;
    wire [31:0] decode_instruction, decode_original;
    wire [7:0] occupancy;
    wire canonical_producer_valid, canonical_producer_is_rvc;
    wire canonical_producer_exception, canonical_carry_valid;
    wire [63:0] canonical_producer_pc, canonical_producer_cause, canonical_carry_pc;
    wire [31:0] canonical_producer_original;
    integer cases = 0, i;
    integer request_count = 0;
    reg [128:0] req, last_request;
    reg response_was_read = 0;
    reg [7:0] completed_count;

    synth_fetch_buffer_integration_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din), .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write), .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n), .imem_resp_in_read(imem_resp_in_read),
        .runtime_reset(runtime_reset), .generic_flush(generic_flush),
        .decode_ready(decode_ready), .canonical_redirect_valid(canonical_redirect_valid),
        .canonical_redirect_target(canonical_redirect_target),
        .push_valid(push_valid), .push_pc(push_pc),
        .push_instruction(push_instruction), .push_original(push_original),
        .push_fetch_id(push_fetch_id), .push_is_rvc(push_is_rvc),
        .push_exception(push_exception), .push_cause(push_cause),
        .push_held(push_held), .decode_valid(decode_valid), .decode_pc(decode_pc),
        .decode_instruction(decode_instruction), .decode_original(decode_original),
        .decode_is_rvc(decode_is_rvc), .decode_exception(decode_exception),
        .decode_cause(decode_cause), .occupancy(occupancy), .full(full),
        .canonical_producer_valid(canonical_producer_valid),
        .canonical_producer_pc(canonical_producer_pc),
        .canonical_producer_original(canonical_producer_original),
        .canonical_producer_is_rvc(canonical_producer_is_rvc),
        .canonical_producer_exception(canonical_producer_exception),
        .canonical_producer_cause(canonical_producer_cause),
        .canonical_carry_valid(canonical_carry_valid),
        .canonical_carry_pc(canonical_carry_pc)
    );

    always #5 ap_clk = ~ap_clk;
    always @(posedge ap_clk) begin
        if (imem_req_out_write && imem_req_out_full_n) begin
            last_request = imem_req_out_din;
            request_count = request_count + 1;
        end
        if (imem_resp_in_read && imem_resp_in_empty_n)
            response_was_read = 1;
    end

    task invoke;
        integer guard;
        begin
            @(negedge ap_clk); ap_start = 1;
            @(negedge ap_clk); ap_start = 0; guard = 0;
            while (ap_done !== 1'b1 && guard < 100) begin
                @(negedge ap_clk); guard = guard + 1;
            end
            if (ap_done !== 1'b1) $fatal(1, "integration RTL timeout");
            #1;
        end
    endtask

    function [224:0] response_payload;
        input [128:0] request;
        input [31:0] instruction;
        input exception;
        input [63:0] cause;
        response_payload = {cause, exception, instruction, request[127:96],
                            request[95:64], request[63:0]};
    endfunction

    task respond(input [31:0] instruction, input exception, input [63:0] cause,
                 input [8*80-1:0] handshake_name);
        begin
            req = last_request;
            imem_resp_in_dout = response_payload(req, instruction, exception, cause);
            imem_resp_in_empty_n = 1;
            response_was_read = 0;
            invoke();
            imem_resp_in_empty_n = 0;
            check(response_was_read, handshake_name);
        end
    endtask

    task semantic_check(input condition, input [8*80-1:0] name);
        begin
            if (!condition) begin
                $display("CASE_FAIL,%0s", name); $fatal(1, "%0s", name);
            end
            cases = cases + 1;
            $display("SEMANTIC_PASS,%0s", name);
            $display("CASE_PASS,%0s,real frontend_module IMEM request/response semantics", name);
        end
    endtask

    task check(input condition, input [8*80-1:0] name);
        begin
            if (!condition) begin
                $display("CASE_FAIL,%0s", name); $fatal(1, "%0s", name);
            end
            cases = cases + 1; $display("CASE_PASS,%0s,canonical integration", name);
        end
    endtask

    task push(input [63:0] pc, input [31:0] inst, input rvc,
              input fault, input [63:0] cause);
        begin
            push_valid = 1; push_pc = pc; push_instruction = inst;
            push_original = rvc ? 32'h00000001 : inst;
            push_fetch_id = pc[33:2]; push_is_rvc = rvc;
            push_exception = fault; push_cause = cause; invoke(); push_valid = 0;
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk); ap_rst = 0;
        runtime_reset = 1; invoke(); runtime_reset = 0;
        check(occupancy == 0, "reset_empty");
        check(!decode_valid, "reset_no_decode");

        canonical_redirect_target = 64'h80002;
        canonical_redirect_valid = 1; invoke(); canonical_redirect_valid = 0;
        check(last_request[63:0] == 64'h80000,
              "canonical_upper_half_request");
        respond(32'h00010013, 0, 0, "upper_half_response_consumed");
        semantic_check(canonical_producer_valid && canonical_producer_is_rvc &&
                       canonical_producer_original == 16'h0001,
                       "upper_half_rvc_production");
        semantic_check(canonical_producer_pc == 64'h80002,
                       "rvc_pc_plus_2_start");
        check(last_request[63:0] == 64'h80004,
              "post_rvc_cross_word_request");
        respond(32'h00930001, 0, 0, "cross_setup_response_consumed");
        check(canonical_producer_valid && canonical_producer_is_rvc &&
              canonical_producer_pc == 64'h80004, "lower_half_setup_rvc");
        completed_count = occupancy;
        invoke();
        semantic_check(canonical_carry_valid && canonical_carry_pc == 64'h80006,
                       "cross_word_carry_creation");
        semantic_check(!canonical_producer_valid && occupancy == completed_count + 1,
                       "partial_cross_word_excluded_from_fetch_buffer");
        check(last_request[63:0] == 64'h80008,
              "cross_word_completion_request");
        respond(0, 1, 12, "cross_fault_response_consumed");
        semantic_check(canonical_producer_valid && canonical_producer_exception &&
                       canonical_producer_pc == 64'h80006 && canonical_producer_cause == 12,
                       "faulted_cross_word_lower_half_start_pc");

        runtime_reset = 1; invoke(); runtime_reset = 0;
        check(occupancy == 0 && !canonical_producer_valid && !canonical_carry_valid,
              "canonical_sequence_reset_flush");

        push(64'h1000, 32'h00108093, 0, 0, 0);
        check(!push_held, "lane0_accept_first_cycle");
        check(occupancy == 1, "first_cycle_occupancy");
        invoke();
        check(occupancy == 1, "lane0_enqueue");
        check(!push_held, "producer_release");
        invoke();
        check(!decode_valid && occupancy == 1, "decode_stall_retains_head");
        decode_ready = 1; invoke(); decode_ready = 0;
        check(decode_valid, "head_visible_on_pop");
        check(decode_pc == 64'h1000, "head_pc");
        check(decode_instruction == 32'h00108093, "head_instruction");
        check(decode_valid, "decode_pop_valid");
        check(occupancy == 0, "decode_pop_removes");

        for (i = 0; i < 8; i = i + 1) begin
            push(64'h2000 + i*4, 32'h00000013 + i, i[0], 0, 0);
            invoke();
        end
        check(occupancy == 8, "fill_depth8");
        check(full, "full_flag");
        push(64'h3000, 32'h12300013, 0, 0, 0);
        check(push_held, "full_backpressure");
        invoke(); check(push_held, "full_hold_stable");
        decode_ready = 1; invoke(); decode_ready = 0;
        check(decode_pc == 64'h2000, "fifo_first");
        check(occupancy == 8, "same_cycle_pop_push_capacity");
        check(!push_held, "stall_release");

        decode_ready = 1;
        for (i = 1; i < 8; i = i + 1) begin
            invoke();
            if (decode_pc != 64'h2000 + i*4)
                $fatal(1, "fifo_order_wrap index=%0d", i);
        end
        check(1, "fifo_order_wrap");
        invoke(); check(decode_pc == 64'h3000, "fifo_last_held");
        decode_ready = 0;
        check(occupancy == 0, "drain_empty");

        push(64'h4000, 32'h00000013, 1, 0, 0); invoke();
        decode_ready = 1; invoke(); decode_ready = 0;
        check(decode_is_rvc, "rvc_metadata");
        check(decode_original == 32'h1, "rvc_original");
        generic_flush = 1; decode_ready = 1; invoke(); generic_flush = 0; decode_ready = 0;
        check(occupancy == 0, "redirect_flush");
        check(!decode_valid, "redirect_no_old_pop");

        push(64'h5002, 0, 0, 1, 12); invoke();
        decode_ready = 1; invoke(); decode_ready = 0;
        check(decode_exception, "fault_entry");
        check(decode_cause == 12, "fault_cause");
        generic_flush = 1; invoke(); generic_flush = 0;
        check(!decode_valid, "fault_flush_no_late_exception");

        push(64'h6000, 32'h00100093, 0, 0, 0); invoke();
        runtime_reset = 1; decode_ready = 1; invoke(); runtime_reset = 0; decode_ready = 0;
        check(occupancy == 0, "full_reset_flush");
        check(!decode_valid, "reset_no_post_flush_entry");
        check(!push_held, "reset_kills_producer");
        check(!imem_resp_in_read, "no_spurious_response_read");
        check(cases >= 30, "minimum_30_cases");
        $display("GATE5_3_B2_FETCH_BUFFER_INTEGRATION_RTL_PASS cases=%0d", cases);
        $finish;
    end
endmodule
