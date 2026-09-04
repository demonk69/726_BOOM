`timescale 1ns/1ps

module pf2_predictor_frontend_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg imem_req_out_full_n = 1;
    reg [224:0] imem_resp_in_dout = 0;
    reg imem_resp_in_empty_n = 0;
    reg runtime_reset = 0;
    reg decode_ready = 1;
    reg [7:0] redirect_kind = 0;
    reg [63:0] redirect_target = 0;
    reg diagnostic_train_valid = 0;
    reg diagnostic_train_taken = 0;
    reg [63:0] diagnostic_train_pc = 0;

    wire ap_done, ap_idle, ap_ready;
    wire [128:0] imem_req_out_din;
    wire imem_req_out_write, imem_resp_in_read;
    wire predictor_request_accepted, predictor_response_valid;
    wire prediction_valid, predicted_taken, target_valid;
    wire [63:0] predicted_target;
    wire prediction_pending, stale_response;
    wire [7:0] selected_cfi_lane, selected_cfi_type;
    wire [7:0] original_packet_mask, final_packet_mask;
    wire pending_packet_valid;
    wire [63:0] frontend_pc;
    wire [7:0] fetch_buffer_count;

    reg [128:0] last_request = 0;
    integer request_count = 0;
    integer pass_count = 0;
    integer iteration;

    synth_pf2_predictor_frontend_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din),
        .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write),
        .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n),
        .imem_resp_in_read(imem_resp_in_read),
        .runtime_reset(runtime_reset), .decode_ready(decode_ready),
        .redirect_kind(redirect_kind), .redirect_target(redirect_target),
        .diagnostic_train_valid(diagnostic_train_valid),
        .diagnostic_train_taken(diagnostic_train_taken),
        .diagnostic_train_pc(diagnostic_train_pc),
        .predictor_request_accepted(predictor_request_accepted),
        .predictor_response_valid(predictor_response_valid),
        .prediction_valid(prediction_valid), .predicted_taken(predicted_taken),
        .target_valid(target_valid), .predicted_target(predicted_target),
        .prediction_pending(prediction_pending), .stale_response(stale_response),
        .selected_cfi_lane(selected_cfi_lane),
        .selected_cfi_type(selected_cfi_type),
        .original_packet_mask(original_packet_mask),
        .final_packet_mask(final_packet_mask),
        .pending_packet_valid(pending_packet_valid),
        .frontend_pc(frontend_pc), .fetch_buffer_count(fetch_buffer_count)
    );

    always #5 ap_clk = ~ap_clk;
    always @(posedge ap_clk)
        if (imem_req_out_write && imem_req_out_full_n) begin
            last_request = imem_req_out_din;
            request_count = request_count + 1;
        end

    task fail(input [8*64-1:0] label);
        begin
            $display("CASE_FAIL,%0s", label);
            $fatal(1, "PF2_PREDICTOR_FRONTEND_RTL_FAIL case=%0s", label);
        end
    endtask

    task check(input [8*64-1:0] label, input condition);
        begin
            if (condition !== 1'b1) fail(label);
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s", label);
        end
    endtask

    task invoke;
        integer guard;
        begin
            @(negedge ap_clk); ap_start = 1;
            @(negedge ap_clk); ap_start = 0;
            guard = 0;
            while (ap_done !== 1'b1 && guard < 1200) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1) fail("ap_ctrl_timeout");
        end
    endtask

    function [224:0] response_payload;
        input [128:0] request;
        input [31:0] word;
        input fault;
        response_payload = {fault ? 64'd12 : 64'd0, fault, word,
                            request[127:96], request[95:64], request[63:0]};
    endfunction

    task reset_and_request;
        integer request_count_before;
        begin
            runtime_reset = 1;
            invoke();
            runtime_reset = 0;
            redirect_kind = 0;
            decode_ready = 1;
            request_count_before = request_count;
            invoke();
            if (request_count <= request_count_before) invoke();
            if (request_count <= request_count_before) fail("reset_request_missing");
            invoke();
        end
    endtask

    task train_taken(input [63:0] pc, input integer updates);
        integer n;
        begin
            diagnostic_train_pc = pc;
            diagnostic_train_taken = 1;
            for (n = 0; n < updates; n = n + 1) begin
                diagnostic_train_valid = 1;
                invoke();
            end
            diagnostic_train_valid = 0;
        end
    endtask

    task respond(input [31:0] word, input fault);
        begin
            imem_resp_in_dout = response_payload(last_request, word, fault);
            imem_resp_in_empty_n = 1;
            invoke();
            imem_resp_in_empty_n = 0;
        end
    endtask

    initial begin
        repeat (6) @(negedge ap_clk);
        ap_rst = 0;

        for (iteration = 0; iteration < 16; iteration = iteration + 1) begin
            reset_and_request();
            case (iteration % 5)
            0: begin
                respond(32'h00010001, 0);
                check("no_cfi_pending", pending_packet_valid);
                check("no_cfi_mask11", final_packet_mask == 3);
                check("no_cfi_no_predict_wait", !prediction_pending);
                check("no_cfi_no_predict_request", !predictor_request_accepted);
                invoke();
                check("no_cfi_atomic_admit", fetch_buffer_count == 2);
            end
            1: begin
                respond(32'h0001c001, 0);
                check("conditional_request_N", predictor_request_accepted);
                check("conditional_pending_N", prediction_pending);
                check("conditional_mask11_N", final_packet_mask == 3);
                check("conditional_no_response_N", !predictor_response_valid);
                invoke();
                check("conditional_response_N1", predictor_response_valid);
                check("conditional_WN", prediction_valid && !predicted_taken);
                check("conditional_shadow_mask", fetch_buffer_count == 2);
                check("conditional_shadow_pc", frontend_pc == 64'h00010044);
            end
            2: begin
                train_taken(64'h00010040, 2);
                respond(32'h0001c001, 0);
                check("conditional_repeat_request", predictor_request_accepted);
                check("conditional_repeat_original11", original_packet_mask == 3);
                check("conditional_repeat_final11", final_packet_mask == 3);
                invoke();
                check("conditional_repeat_response", predictor_response_valid);
                check("conditional_repeat_ST", prediction_valid && predicted_taken);
                check("conditional_repeat_shadow_pc", frontend_pc == 64'h00010044);
                check("conditional_repeat_younger_kept", fetch_buffer_count == 2);
            end
            3: begin
                respond(32'h0001a001, 0);
                check("jal_lane0_type", selected_cfi_type == 2);
                check("jal_lane0_selected", selected_cfi_lane == 0);
                check("jal_lane0_original11", original_packet_mask == 3);
                check("jal_lane0_final01", final_packet_mask == 1);
                check("jal_lane0_target", frontend_pc == 64'h00010040);
                invoke();
                check("jal_lane0_single_admit", fetch_buffer_count == 1);
            end
            default: begin
                respond(32'h80820001, 0);
                check("jalr_lane1_type", selected_cfi_type == 3);
                check("jalr_lane1_selected", selected_cfi_lane == 1);
                check("jalr_no_predict_request", !predictor_request_accepted);
                check("jalr_no_prediction", !prediction_valid);
                check("jalr_mask11", final_packet_mask == 3);
                check("jalr_fallthrough", frontend_pc == 64'h00010044);
                invoke();
                check("jalr_atomic_admit", fetch_buffer_count == 2);
            end
            endcase
        end

        reset_and_request();
        respond(32'h0001c001, 0);
        check("redirect_pending_request", prediction_pending);
        redirect_kind = 2;
        redirect_target = 64'h80002000;
        invoke();
        redirect_kind = 0;
        check("exception_over_prediction", frontend_pc == 64'h80002000);
        check("redirect_clears_pending", !prediction_pending);
        check("redirect_drains_stale", stale_response);
        check("redirect_blocks_enqueue", fetch_buffer_count == 0);

        reset_and_request();
        respond(32'h0000a001, 0);
        check("jal_before_fault_type", selected_cfi_type == 2);
        check("jal_before_fault_lane0", selected_cfi_lane == 0);
        check("jal_before_fault_original11", original_packet_mask == 3);
        check("jal_before_fault_final01", final_packet_mask == 1);
        check("jal_before_fault_target", frontend_pc == 64'h00010040);
        check("jal_before_fault_no_prediction", !prediction_pending);
        invoke();
        check("jal_before_fault_single_admit", fetch_buffer_count == 1);

        if (pass_count != 116) fail("mandatory_exact_case_count");
        $display("PF2_PREDICTOR_FRONTEND_RTL_PASS cases=%0d", pass_count);
        $finish;
    end
endmodule
