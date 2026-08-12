`timescale 1ns/1ps

module rvc_frontend_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg imem_req_out_full_n = 1;
    reg [224:0] imem_resp_in_dout = 0;
    reg imem_resp_in_empty_n = 0;
    reg runtime_reset = 0;
    reg decode_ready = 1;
    reg redirect_valid = 0;
    reg [63:0] redirect_target = 0;

    wire ap_done, ap_idle, ap_ready;
    wire [128:0] imem_req_out_din;
    wire imem_req_out_write, imem_resp_in_read;
    wire decode_valid, decode_valid_ap_vld;
    wire [63:0] decode_pc, decode_fault_cause, frontend_pc, expected_address, carry_pc;
    wire decode_pc_ap_vld, decode_fault_cause_ap_vld, frontend_pc_ap_vld;
    wire expected_address_ap_vld, carry_pc_ap_vld;
    wire [31:0] decode_instruction, frontend_epoch, carry_epoch;
    wire decode_instruction_ap_vld, frontend_epoch_ap_vld, carry_epoch_ap_vld;
    wire decode_fault, decode_fault_ap_vld, decode_is_rvc, decode_is_rvc_ap_vld;
    wire [15:0] decode_original_bits, carry_value;
    wire decode_original_bits_ap_vld, carry_value_ap_vld;
    wire held_entry_valid, held_entry_valid_ap_vld;
    wire outstanding_valid, outstanding_valid_ap_vld;
    wire carry_valid, carry_valid_ap_vld;
    wire accepted_response_pulse, accepted_response_pulse_ap_vld;
    wire stale_response_pulse, stale_response_pulse_ap_vld;
    wire redirect_accepted_pulse, redirect_accepted_pulse_ap_vld;
    wire misalignment_fault_pulse, misalignment_fault_pulse_ap_vld;

    integer pass_count = 0;
    integer request_count = 0;
    integer response_count = 0;
    integer before_requests;
    integer i;
    reg response_was_read = 0;
    reg [128:0] requests [0:255];
    reg [128:0] req, upper_req, old_req;
    reg [63:0] held_pc;
    reg [31:0] held_inst, held_epoch;

    synth_r2_rvc_frontend_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din),
        .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write),
        .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n),
        .imem_resp_in_read(imem_resp_in_read),
        .runtime_reset(runtime_reset), .decode_ready(decode_ready),
        .redirect_valid(redirect_valid), .redirect_target(redirect_target),
        .decode_valid(decode_valid), .decode_valid_ap_vld(decode_valid_ap_vld),
        .decode_pc(decode_pc), .decode_pc_ap_vld(decode_pc_ap_vld),
        .decode_instruction(decode_instruction),
        .decode_instruction_ap_vld(decode_instruction_ap_vld),
        .decode_fault(decode_fault), .decode_fault_ap_vld(decode_fault_ap_vld),
        .decode_fault_cause(decode_fault_cause),
        .decode_fault_cause_ap_vld(decode_fault_cause_ap_vld),
        .decode_is_rvc(decode_is_rvc), .decode_is_rvc_ap_vld(decode_is_rvc_ap_vld),
        .decode_original_bits(decode_original_bits),
        .decode_original_bits_ap_vld(decode_original_bits_ap_vld),
        .held_entry_valid(held_entry_valid),
        .held_entry_valid_ap_vld(held_entry_valid_ap_vld),
        .outstanding_valid(outstanding_valid),
        .outstanding_valid_ap_vld(outstanding_valid_ap_vld),
        .frontend_pc(frontend_pc), .frontend_pc_ap_vld(frontend_pc_ap_vld),
        .frontend_epoch(frontend_epoch), .frontend_epoch_ap_vld(frontend_epoch_ap_vld),
        .expected_address(expected_address), .expected_address_ap_vld(expected_address_ap_vld),
        .carry_valid(carry_valid), .carry_valid_ap_vld(carry_valid_ap_vld),
        .carry_value(carry_value), .carry_value_ap_vld(carry_value_ap_vld),
        .carry_pc(carry_pc), .carry_pc_ap_vld(carry_pc_ap_vld),
        .carry_epoch(carry_epoch), .carry_epoch_ap_vld(carry_epoch_ap_vld),
        .accepted_response_pulse(accepted_response_pulse),
        .accepted_response_pulse_ap_vld(accepted_response_pulse_ap_vld),
        .stale_response_pulse(stale_response_pulse),
        .stale_response_pulse_ap_vld(stale_response_pulse_ap_vld),
        .redirect_accepted_pulse(redirect_accepted_pulse),
        .redirect_accepted_pulse_ap_vld(redirect_accepted_pulse_ap_vld),
        .misalignment_fault_pulse(misalignment_fault_pulse),
        .misalignment_fault_pulse_ap_vld(misalignment_fault_pulse_ap_vld)
    );

    always #5 ap_clk = ~ap_clk;
    always @(posedge ap_clk) begin
        if (imem_req_out_write && imem_req_out_full_n) begin
            requests[request_count] = imem_req_out_din;
            request_count = request_count + 1;
        end
        if (imem_resp_in_read && imem_resp_in_empty_n) begin
            response_was_read = 1;
            response_count = response_count + 1;
        end
    end

    function [224:0] response_payload;
        input [63:0] address;
        input [31:0] fetch_id;
        input [31:0] epoch;
        input [31:0] instruction;
        input exception;
        input [63:0] cause;
        response_payload = {cause, exception, instruction, epoch, fetch_id, address};
    endfunction

    task fail(input [8*80-1:0] name, input [8*160-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "GATE5_2_R2_RVC_FRONTEND_RTL_FAIL case=%0s", name);
        end
    endtask

    task pass(input [8*80-1:0] name, input [8*180-1:0] requirement);
        begin
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task invoke(input has_response, input [224:0] response);
        integer guard;
        begin
            @(negedge ap_clk);
            response_was_read = 0;
            imem_resp_in_dout = response;
            imem_resp_in_empty_n = has_response;
            ap_start = 1;
            @(negedge ap_clk);
            ap_start = 0;
            guard = 0;
            while (ap_done !== 1'b1 && guard < 300) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1) fail("generated_control_timeout", "ap_done did not arrive");
            if (has_response && !response_was_read)
                fail("response_fifo_protocol", "present response was not drained");
            imem_resp_in_empty_n = 0;
            @(posedge ap_clk);
            #1;
        end
    endtask

    task redirect_to(input [63:0] target);
        begin
            redirect_target = target;
            redirect_valid = 1;
            invoke(0, 0);
            redirect_valid = 0;
            req = requests[request_count-1];
        end
    endtask

    task expect_stale(input [8*80-1:0] name, input [224:0] response);
        reg [63:0] pc_before;
        integer request_before;
        begin
            pc_before = frontend_pc;
            request_before = request_count;
            invoke(1, response);
            if (!stale_response_pulse || accepted_response_pulse ||
                frontend_pc !== pc_before || request_count != request_before)
                fail(name, "stale response changed canonical pending state");
            pass(name, "wrong identity response drained with no architectural effect");
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0;

        invoke(0, 0);
        req = requests[0];
        if (request_count != 1 || req[63:0] !== 64'h10040 || req[95:64] !== 0 ||
            req[127:96] !== 0 || req[128] || !outstanding_valid)
            fail("initial_request", "reset-vector transaction mismatch");
        pass("initial_request", "canonical reset issued one aligned fetch with fetch_id 0 epoch 0");

        expect_stale("wrong_fetch_id_lower", response_payload(req[63:0], req[95:64]+1,
                     req[127:96], 32'h00850001, 0, 0));
        expect_stale("wrong_epoch_lower", response_payload(req[63:0], req[95:64],
                     req[127:96]+1, 32'h00850001, 0, 0));
        expect_stale("wrong_address_lower", response_payload(req[63:0]+4, req[95:64],
                     req[127:96], 32'h00850001, 0, 0));
        before_requests = request_count;
        repeat (3) invoke(0, 0);
        if (!outstanding_valid || request_count != before_requests)
            fail("delayed_lower_response", "pending request was not stable");
        pass("delayed_lower_response", "delayed matching response preserved one outstanding request");

        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00850001, 0, 0));
        if (!accepted_response_pulse || !decode_valid || decode_fault)
            fail("basic_c_lower_valid", "lower compressed parcel was not published");
        pass("basic_c_lower_valid", "lower C.NOP produced one valid Decode entry");
        if (decode_pc !== 64'h10040) fail("basic_c_lower_pc", "C.NOP PC mismatch");
        pass("basic_c_lower_pc", "lower compressed parcel retained exact PC");
        if (decode_instruction !== 32'h00000013) fail("basic_c_expansion", "C.NOP expansion mismatch");
        pass("basic_c_expansion", "C.NOP expanded to canonical ADDI x0 x0 0");
        if (!decode_is_rvc) fail("basic_c_attribution", "RVC flag missing");
        pass("basic_c_attribution", "compressed attribution remained observable");
        if (decode_original_bits !== 16'h0001) fail("basic_c_original_bits", "parcel bits mismatch");
        pass("basic_c_original_bits", "original compressed parcel remained observable");
        if (frontend_pc !== 64'h10042) fail("basic_c_pc_plus_2", "frontend PC did not add two");
        pass("basic_c_pc_plus_2", "compressed instruction advanced frontend PC by two");

        invoke(0, 0);
        if (!decode_valid || decode_pc !== 64'h10042 || decode_instruction !== 32'h00108093)
            fail("upper_c_same_word", "retained upper compressed parcel mismatch");
        pass("upper_c_same_word", "upper C.ADDI was consumed from retained response word");
        if (!decode_is_rvc || decode_original_bits !== 16'h0085)
            fail("upper_c_original_bits", "upper parcel attribution mismatch");
        pass("upper_c_original_bits", "upper compressed original bits were exact");
        if (frontend_pc !== 64'h10044) fail("upper_c_pc_plus_2", "upper C PC increment mismatch");
        pass("upper_c_pc_plus_2", "second compressed instruction advanced PC by two");
        req = requests[request_count-1];

        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00200113, 0, 0));
        if (!decode_valid || decode_instruction !== 32'h00200113 || decode_pc !== 64'h10044)
            fail("basic_32_aligned", "aligned instruction mismatch");
        pass("basic_32_aligned", "aligned 32-bit instruction published unchanged");
        if (frontend_pc !== 64'h10048) fail("basic_32_pc_plus_4", "32-bit PC increment mismatch");
        pass("basic_32_pc_plus_4", "32-bit instruction advanced frontend PC by four");
        if (decode_is_rvc) fail("basic_32_not_rvc", "32-bit instruction marked compressed");
        pass("basic_32_not_rvc", "32-bit instruction was not attributed as compressed");
        pass("mixed_c_c_32", "C/C/32 mixed sequence preserved exact parcel ordering");

        redirect_to(64'h20000);
        if (!redirect_accepted_pulse || req[63:0] !== 64'h20000)
            fail("mixed_cross_redirect", "cross test redirect failed");
        pass("mixed_cross_redirect", "redirect established an aligned mixed-stream base");
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00930001, 0, 0));
        if (!decode_valid || !decode_is_rvc || decode_pc !== 64'h20000)
            fail("mixed_c_before_cross32", "leading C parcel mismatch");
        pass("mixed_c_before_cross32", "compressed parcel preceded a cross-boundary 32-bit instruction");
        invoke(0, 0);
        upper_req = requests[request_count-1];
        if (!carry_valid) fail("cross_carry_valid", "carry was not retained");
        pass("cross_carry_valid", "upper-position 32-bit low half established carry valid");
        if (carry_value !== 16'h0093) fail("cross_carry_value", "carry value mismatch");
        pass("cross_carry_value", "carry retained exact low halfword");
        if (carry_pc !== 64'h20002) fail("cross_carry_pc", "carry PC mismatch");
        pass("cross_carry_pc", "carry retained the architectural instruction PC");
        if (carry_epoch !== frontend_epoch) fail("cross_carry_epoch", "carry epoch mismatch");
        pass("cross_carry_epoch", "carry retained the active frontend epoch");
        expect_stale("wrong_fetch_id_upper", response_payload(upper_req[63:0],
                     upper_req[95:64]+1, upper_req[127:96], 32'h00001230, 0, 0));
        expect_stale("wrong_epoch_upper", response_payload(upper_req[63:0],
                     upper_req[95:64], upper_req[127:96]+1, 32'h00001230, 0, 0));
        expect_stale("wrong_address_upper", response_payload(upper_req[63:0]+4,
                     upper_req[95:64], upper_req[127:96], 32'h00001230, 0, 0));
        invoke(1, response_payload(upper_req[63:0], upper_req[95:64], upper_req[127:96],
                                   32'h00001230, 0, 0));
        if (!decode_valid || decode_instruction !== 32'h12300093 || decode_is_rvc)
            fail("cross_assembled_instruction", "cross-word assembly mismatch");
        pass("cross_assembled_instruction", "matching delayed upper assembled exact 32-bit instruction");
        if (decode_pc !== 64'h20002) fail("cross_assembled_pc", "assembled PC mismatch");
        pass("cross_assembled_pc", "assembled instruction retained low-halfword PC");
        if (carry_valid) fail("cross_carry_consumed", "carry survived assembly");
        pass("cross_carry_consumed", "matching upper consumed carry exactly once");
        if (frontend_pc !== 64'h20006) fail("cross_pc_plus_4", "assembled PC increment mismatch");
        pass("cross_pc_plus_4", "cross-boundary 32-bit instruction advanced PC by four");

        held_pc = decode_pc; held_inst = decode_instruction;
        decode_ready = 0;
        invoke(0, 0);
        if (!decode_valid || !held_entry_valid) fail("decode_hold_valid", "held entry dropped");
        pass("decode_hold_valid", "Decode backpressure retained valid fetch entry");
        if (decode_pc !== held_pc || decode_instruction !== held_inst)
            fail("decode_hold_stability", "held payload changed");
        pass("decode_hold_stability", "held PC and instruction remained stable");
        invoke(1, response_payload(upper_req[63:0], upper_req[95:64], upper_req[127:96],
                                   32'h00001230, 0, 0));
        if (!stale_response_pulse) fail("duplicate_upper_stale", "duplicate was accepted");
        pass("duplicate_upper_stale", "duplicate consumed upper response was classified stale");
        if (!decode_valid || decode_instruction !== held_inst || !held_entry_valid)
            fail("duplicate_upper_no_effect", "duplicate changed held publication");
        pass("duplicate_upper_no_effect", "duplicate response did not alter held Decode publication");

        before_requests = request_count;
        redirect_target = 64'h20f00; redirect_valid = 1;
        invoke(0, 0);
        redirect_valid = 0;
        if (decode_valid || held_entry_valid)
            fail("redirect_kills_held_decode", "redirect retained stale backpressured Decode entry");
        pass("redirect_kills_held_decode", "redirect invalidated stale held Decode publication");
        if (request_count != before_requests + 1 || requests[request_count-1][63:0] !== 64'h20f00)
            fail("redirect_after_hold_request", "redirect after Decode hold requested wrong target");
        pass("redirect_after_hold_request", "redirect after Decode hold issued only its target request");
        decode_ready = 1;

        redirect_to(64'h21002);
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00930001, 0, 0));
        upper_req = requests[request_count-1];
        if (!carry_valid) fail("cross_fault_setup", "fault carry setup failed");
        invoke(1, response_payload(upper_req[63:0], upper_req[95:64], upper_req[127:96],
                                   0, 1, 64'd11));
        if (!decode_valid || !decode_fault || decode_fault_cause !== 11)
            fail("cross_upper_fault", "upper access fault mismatch");
        pass("cross_upper_fault", "matching cross upper access fault reached Decode");
        if (decode_pc !== 64'h21002) fail("cross_upper_fault_pc", "fault PC mismatch");
        pass("cross_upper_fault_pc", "cross upper fault was attributed to carried instruction PC");
        if (carry_valid) fail("cross_fault_carry_clear", "fault retained carry");
        pass("cross_fault_carry_clear", "cross upper fault consumed and cleared carry");

        redirect_to(64'h22002);
        if (req[63:0] !== 64'h22000 || misalignment_fault_pulse)
            fail("redirect_mod4_2_request", "PC mod 4 equals 2 redirect policy mismatch");
        pass("redirect_mod4_2_request", "PC mod 4 equals 2 redirected to aligned containing word without fault");
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00010013, 0, 0));
        if (!decode_valid || !decode_is_rvc || decode_pc !== 64'h22002)
            fail("redirect_mod4_2_decode", "upper redirected parcel mismatch");
        pass("redirect_mod4_2_decode", "redirected upper compressed parcel retained exact target PC");

        before_requests = request_count;
        redirect_to(64'h23003);
        if (!misalignment_fault_pulse) fail("odd_redirect_pulse", "misalignment pulse missing");
        pass("odd_redirect_pulse", "odd branch redirect asserted misalignment pulse");
        if (!decode_valid || !decode_fault || decode_pc !== 64'h23003 || decode_fault_cause !== 0)
            fail("odd_redirect_fault", "odd redirect fault payload mismatch");
        pass("odd_redirect_fault", "odd redirect published precise MA-IF Decode fault");
        if (request_count != before_requests) fail("odd_redirect_no_request", "odd redirect fetched memory");
        pass("odd_redirect_no_request", "odd redirect issued no IMEM request");
        invoke(0, 0);
        if (request_count != before_requests || frontend_pc !== 64'h23003)
            fail("odd_redirect_persistent_fence", "odd PC fetched after fault consumption");
        pass("odd_redirect_persistent_fence", "odd PC remained fenced until a later redirect");

        redirect_to(64'h24002);
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00930001, 0, 0));
        old_req = requests[request_count-1];
        held_epoch = frontend_epoch;
        redirect_target = 64'h25000; redirect_valid = 1;
        invoke(1, response_payload(old_req[63:0], old_req[95:64], old_req[127:96],
                                   32'h00001230, 0, 0));
        redirect_valid = 0;
        if (carry_valid || frontend_pc !== 64'h25000)
            fail("redirect_clears_partial", "redirect did not clear carry");
        pass("redirect_clears_partial", "redirect while partial instruction cleared carry");
        if (!stale_response_pulse || accepted_response_pulse || frontend_epoch !== held_epoch+1)
            fail("redirect_rejects_stale_upper", "redirect-cycle upper response leaked");
        pass("redirect_rejects_stale_upper", "redirect won over stale upper and advanced epoch");

        req = requests[request_count-1];
        redirect_to(64'h26002);
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00930001, 0, 0));
        old_req = requests[request_count-1];
        runtime_reset = 1;
        invoke(1, response_payload(old_req[63:0], old_req[95:64], old_req[127:96],
                                   32'h00001230, 0, 0));
        runtime_reset = 0;
        if (carry_valid || frontend_pc !== 64'h10040)
            fail("reset_clears_partial", "runtime reset retained partial state");
        pass("reset_clears_partial", "runtime reset cleared partial instruction and restarted PC");
        if (!stale_response_pulse || accepted_response_pulse)
            fail("reset_rejects_stale_upper", "reset-cycle upper response leaked");
        pass("reset_rejects_stale_upper", "runtime reset drained old upper without publication");

        req = requests[request_count-1];
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00019002, 0, 0));
        if (!decode_valid || !decode_fault || decode_fault_cause !== 2 || !decode_is_rvc)
            fail("protected_c_ebreak", "C.EBREAK protection mismatch");
        pass("protected_c_ebreak", "protected C.EBREAK gap faulted as illegal RVC");
        if (decode_original_bits !== 16'h9002)
            fail("protected_c_ebreak_bits", "C.EBREAK bits mismatch");
        pass("protected_c_ebreak_bits", "protected C.EBREAK retained original compressed bits");

        redirect_to(64'h27000);
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00019001, 0, 0));
        if (!decode_valid || !decode_fault || decode_fault_cause !== 2 || !decode_is_rvc)
            fail("protected_c_srli_shamt5", "C.SRLI protection mismatch");
        pass("protected_c_srli_shamt5", "protected RV64 C.SRLI shamt5 gap faulted as illegal RVC");
        if (decode_original_bits !== 16'h9001)
            fail("protected_c_srli_bits", "C.SRLI bits mismatch");
        pass("protected_c_srli_bits", "protected C.SRLI retained original compressed bits");

        redirect_to(64'h27500);
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00019782, 0, 0));
        if (!decode_valid || !decode_fault || decode_fault_cause !== 2 || !decode_is_rvc)
            fail("protected_c_jalr", "C.JALR PC+2-link protection mismatch");
        pass("protected_c_jalr", "C.JALR faulted rather than using frozen backend PC+4 link");
        if (decode_original_bits !== 16'h9782)
            fail("protected_c_jalr_bits", "C.JALR bits mismatch");
        pass("protected_c_jalr_bits", "protected C.JALR retained original compressed bits");

        redirect_to(64'h28000);
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h0000001f, 0, 0));
        if (!decode_valid || !decode_fault || decode_fault_cause !== 2 ||
            decode_is_rvc || frontend_pc !== 64'h28002)
            fail("reserved_long_encoding", "reserved long encoding policy mismatch");
        pass("reserved_long_encoding", "reserved long encoding faulted and advanced by one parcel");

        redirect_to(64'h29000);
        req = requests[request_count-1];
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00300193, 0, 0));
        req = requests[request_count-1];
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                   32'h00010001, 0, 0));
        if (!decode_valid || !decode_is_rvc || decode_pc !== 64'h29004)
            fail("sequence_32_then_c", "32/C ordering mismatch");
        pass("sequence_32_then_c", "32-bit then compressed sequence preserved PC plus four then plus two");

        if (pass_count != 58) fail("matrix_count", "expected exactly 58 unique observable cases");
        $display("GATE5_2_R2_RVC_FRONTEND_RTL_PASS cases=%0d requests=%0d responses=%0d",
                 pass_count, request_count, response_count);
        $finish;
    end
endmodule
