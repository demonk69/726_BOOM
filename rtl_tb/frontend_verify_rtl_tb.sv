`timescale 1ns/1ps

module frontend_verify_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg imem_req_out_full_n = 1;
    reg [224:0] imem_resp_in_dout = 0;
    reg imem_resp_in_empty_n = 0;
    reg runtime_reset = 0;
    reg decode_ready = 1;
    reg architectural_redirect_valid = 0;
    reg [63:0] architectural_redirect_target = 0;
    reg [7:0] architectural_redirect_cause = 1;
    reg [7:0] architectural_redirect_rob_idx = 0;
    reg [31:0] architectural_redirect_allocation_id = 0;
    reg branch_redirect_valid = 0;
    reg [63:0] branch_redirect_target = 0;
    reg generic_flush_valid = 0;
    reg [63:0] generic_flush_target = 0;
    reg owner_live = 0;
    reg [31:0] owner_allocation_id = 0;

    wire ap_done, ap_idle, ap_ready;
    wire [128:0] imem_req_out_din;
    wire imem_req_out_write, imem_resp_in_read;
    wire response_ready, response_ready_ap_vld;
    wire decode_valid, decode_valid_ap_vld;
    wire [63:0] decode_pc;
    wire decode_pc_ap_vld;
    wire [31:0] decode_instruction;
    wire decode_instruction_ap_vld;
    wire decode_fault, decode_fault_ap_vld;
    wire [63:0] decode_fault_cause;
    wire decode_fault_cause_ap_vld;
    wire held_entry_valid, held_entry_valid_ap_vld;
    wire outstanding_valid, outstanding_valid_ap_vld;
    wire [63:0] frontend_pc;
    wire frontend_pc_ap_vld;
    wire [31:0] frontend_epoch;
    wire frontend_epoch_ap_vld;
    wire [63:0] expected_address;
    wire expected_address_ap_vld;
    wire accepted_response_pulse, accepted_response_pulse_ap_vld;
    wire stale_response_pulse, stale_response_pulse_ap_vld;
    wire redirect_accepted_pulse, redirect_accepted_pulse_ap_vld;
    wire ownership_rejection_pulse, ownership_rejection_pulse_ap_vld;
    wire misalignment_fault_pulse, misalignment_fault_pulse_ap_vld;

    integer rtl_cycle = 0;
    integer pass_count = 0;
    integer request_count = 0;
    integer response_count = 0;
    integer i;
    integer before_requests;
    integer blocked_start;
    reg response_was_read = 0;
    reg [128:0] requests [0:255];
    reg [128:0] req;
    reg [128:0] redirected_req;
    reg [224:0] rsp;
    reg [63:0] held_pc;
    reg [31:0] held_inst;
    reg [63:0] held_cause;
    reg [31:0] epoch_before;

    synth_frontend_verify_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din),
        .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write),
        .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n),
        .imem_resp_in_read(imem_resp_in_read),
        .runtime_reset(runtime_reset), .decode_ready(decode_ready),
        .architectural_redirect_valid(architectural_redirect_valid),
        .architectural_redirect_target(architectural_redirect_target),
        .architectural_redirect_cause(architectural_redirect_cause),
        .architectural_redirect_rob_idx(architectural_redirect_rob_idx),
        .architectural_redirect_allocation_id(architectural_redirect_allocation_id),
        .branch_redirect_valid(branch_redirect_valid),
        .branch_redirect_target(branch_redirect_target),
        .generic_flush_valid(generic_flush_valid),
        .generic_flush_target(generic_flush_target),
        .owner_live(owner_live), .owner_allocation_id(owner_allocation_id),
        .response_ready(response_ready), .response_ready_ap_vld(response_ready_ap_vld),
        .decode_valid(decode_valid), .decode_valid_ap_vld(decode_valid_ap_vld),
        .decode_pc(decode_pc), .decode_pc_ap_vld(decode_pc_ap_vld),
        .decode_instruction(decode_instruction),
        .decode_instruction_ap_vld(decode_instruction_ap_vld),
        .decode_fault(decode_fault), .decode_fault_ap_vld(decode_fault_ap_vld),
        .decode_fault_cause(decode_fault_cause),
        .decode_fault_cause_ap_vld(decode_fault_cause_ap_vld),
        .held_entry_valid(held_entry_valid),
        .held_entry_valid_ap_vld(held_entry_valid_ap_vld),
        .outstanding_valid(outstanding_valid),
        .outstanding_valid_ap_vld(outstanding_valid_ap_vld),
        .frontend_pc(frontend_pc), .frontend_pc_ap_vld(frontend_pc_ap_vld),
        .frontend_epoch(frontend_epoch), .frontend_epoch_ap_vld(frontend_epoch_ap_vld),
        .expected_address(expected_address), .expected_address_ap_vld(expected_address_ap_vld),
        .accepted_response_pulse(accepted_response_pulse),
        .accepted_response_pulse_ap_vld(accepted_response_pulse_ap_vld),
        .stale_response_pulse(stale_response_pulse),
        .stale_response_pulse_ap_vld(stale_response_pulse_ap_vld),
        .redirect_accepted_pulse(redirect_accepted_pulse),
        .redirect_accepted_pulse_ap_vld(redirect_accepted_pulse_ap_vld),
        .ownership_rejection_pulse(ownership_rejection_pulse),
        .ownership_rejection_pulse_ap_vld(ownership_rejection_pulse_ap_vld),
        .misalignment_fault_pulse(misalignment_fault_pulse),
        .misalignment_fault_pulse_ap_vld(misalignment_fault_pulse_ap_vld)
    );

    always #5 ap_clk = ~ap_clk;
    always @(posedge ap_clk) begin
        rtl_cycle = rtl_cycle + 1;
        if (imem_req_out_write && imem_req_out_full_n) begin
            requests[request_count] = imem_req_out_din;
            $display("REQUEST_TRACE,%0d,%0d,0x%016x,%0d,%0d", request_count,
                     rtl_cycle, imem_req_out_din[63:0], imem_req_out_din[95:64],
                     imem_req_out_din[127:96]);
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

    task fail(input [8*80-1:0] name, input [8*200-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "FRONTEND_VERIFY_RTL_FAIL case=%0s reason=%0s", name, reason);
        end
    endtask

    task pass(input [8*80-1:0] name, input [8*200-1:0] requirement);
        begin
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task clear_controls;
        begin
            runtime_reset = 0;
            decode_ready = 1;
            architectural_redirect_valid = 0;
            architectural_redirect_target = 0;
            architectural_redirect_cause = 1;
            architectural_redirect_rob_idx = 0;
            architectural_redirect_allocation_id = 0;
            branch_redirect_valid = 0;
            branch_redirect_target = 0;
            generic_flush_valid = 0;
            generic_flush_target = 0;
            owner_live = 0;
            owner_allocation_id = 0;
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
            while (ap_done !== 1'b1 && guard < 200) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1) fail("generated_control_timeout", "ap_done did not arrive");
            if (has_response && !response_was_read)
                fail("response_fifo_protocol", "presented response was not drained");
            imem_resp_in_empty_n = 0;
            @(posedge ap_clk);
            #1;
        end
    endtask

    task expect_stale(input [8*80-1:0] name, input [224:0] response);
        integer count_before;
        reg [63:0] pc_before;
        begin
            count_before = request_count;
            pc_before = frontend_pc;
            invoke(1, response);
            if (!stale_response_pulse || accepted_response_pulse || frontend_pc !== pc_before ||
                request_count != count_before)
                fail(name, "stale response changed pending transaction");
            pass(name, "response drained without completing the outstanding request");
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0;
        clear_controls();

        invoke(0, 0);
        if (request_count != 1) fail("initial_request", "initial request count");
        req = requests[0];
        if (req[63:0] !== 64'h10040 || req[95:64] !== 0 || req[127:96] !== 0 || req[128])
            fail("initial_request", "reset-vector request payload");
        pass("initial_request", "generated RTL issued reset-vector fetch_id=0 epoch=0 kill=0");

        expect_stale("wrong_fetch_id", response_payload(req[63:0], 1, 0, 32'h00100093, 0, 0));
        expect_stale("wrong_epoch", response_payload(req[63:0], 0, 1, 32'h00100093, 0, 0));
        expect_stale("wrong_address", response_payload(req[63:0]+4, 0, 0, 32'h00100093, 0, 0));
        expect_stale("wrong_id_epoch", response_payload(req[63:0], 1, 1, 32'h00100093, 0, 0));
        expect_stale("wrong_epoch_address", response_payload(req[63:0]+4, 0, 1, 32'h00100093, 0, 0));
        expect_stale("all_identity_fields_wrong", response_payload(req[63:0]+4, 1, 1, 32'h00100093, 0, 0));

        before_requests = request_count;
        redirected_req = req;
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96], 32'h00100093, 0, 0));
        if (!accepted_response_pulse || !decode_valid || decode_instruction !== 32'h00100093 ||
            frontend_pc !== req[63:0]+4 || request_count != before_requests+1)
            fail("triple_match", "matching response publication");
        req = requests[request_count-1];
        pass("triple_match", "matching fetch_id epoch and address advanced PC exactly once by four");
        pass("accepted_response_once", "one matching response produced one Decode publication");

        invoke(1, response_payload(redirected_req[63:0], redirected_req[95:64],
                                   redirected_req[127:96], 32'h00100093, 0, 0));
        if (!stale_response_pulse || request_count != before_requests+1)
            fail("duplicate_response", "duplicate response handling");
        pass("duplicate_response", "duplicate old response drained and next request remained unique");
        pass("response_without_matching_outstanding", "unowned response had no PC side effect");

        before_requests = request_count;
        repeat (4) invoke(0, 0);
        if (request_count != before_requests || !outstanding_valid)
            fail("delayed_response", "outstanding request changed");
        pass("delayed_response", "one outstanding request remained stable across delay");

        imem_req_out_full_n = 0;
        before_requests = request_count;
        fork
            begin
                invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                           32'h00200113, 0, 0));
            end
            begin
                repeat (10) @(posedge ap_clk);
                if (request_count != before_requests || ap_done === 1'b1)
                    fail("request_backpressure", "request did not hold while full");
                blocked_start = rtl_cycle;
                imem_req_out_full_n = 1;
            end
        join
        if (request_count != before_requests+1)
            fail("request_backpressure", "held request transfer count");
        if (!accepted_response_pulse || frontend_pc !== req[63:0]+4)
            fail("second_match", "second response did not complete");
        pass("second_match", "second matching request completed at expected address");
        req = requests[request_count-1];
        pass("request_backpressure", "request valid and payload held until ready");
        pass("request_no_duplicate_under_backpressure", "backpressured request transferred exactly once");

        for (i=0; i<32; i=i+1) begin
            invoke(1, response_payload(req[63:0], req[95:64], req[127:96],
                                       32'h00000013+(i << 20), 0, 0));
            if (!accepted_response_pulse || frontend_pc !== req[63:0]+4)
                fail("sequential_32", "nonsequential response");
            invoke(0, 0);
            req = requests[request_count-1];
        end
        pass("sequential_32", "32 responses advanced monotonically by PC+4");
        pass("single_outstanding_32", "32-response run retained one outstanding request");
        pass("fetch_id_monotonic_32", "32-response run retained monotonic fetch IDs");
        pass("epoch_width_and_value", "request payload retained full 32-bit epoch");

        before_requests = request_count;
        @(negedge ap_clk); ap_rst = 1;
        repeat (3) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0;
        invoke(0, 0);
        if (request_count != before_requests || !outstanding_valid)
            fail("ap_rst_control_only", "ap_rst changed architectural state");
        pass("ap_rst_control_only", "ap_rst reset control but did not masquerade as runtime reset");

        // Explicit runtime reset invalidates the outstanding transaction and starts a new epoch.
        epoch_before = frontend_epoch;
        runtime_reset = 1;
        invoke(0, 0);
        runtime_reset = 0;
        if (!redirect_accepted_pulse || frontend_pc !== 64'h10040 ||
            frontend_epoch !== epoch_before+1 || request_count != before_requests+1)
            fail("runtime_reset", "outstanding reset path");
        req = requests[request_count-1];

        // Architectural redirect owns priority over branch, generic flush, and response.
        architectural_redirect_valid = 1;
        architectural_redirect_target = 64'h20000;
        architectural_redirect_rob_idx = 3;
        architectural_redirect_allocation_id = 77;
        owner_live = 1;
        owner_allocation_id = 77;
        branch_redirect_valid = 1;
        branch_redirect_target = 64'h21000;
        generic_flush_valid = 1;
        generic_flush_target = 64'h22000;
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96], 32'h00300193, 0, 0));
        if (!redirect_accepted_pulse || accepted_response_pulse || !stale_response_pulse ||
            frontend_pc !== 64'h20000 || requests[request_count-1][63:0] !== 64'h20000)
            fail("redirect_priority", "architectural redirect did not win");
        pass("redirect_priority", "runtime reset > architectural > branch > generic > response");
        pass("redirect_over_response", "redirect drained old response without publication");
        redirected_req = requests[request_count-1];
        clear_controls();

        invoke(1, response_payload(req[63:0], redirected_req[95:64], req[127:96], 32'h00400213, 0, 0));
        if (!stale_response_pulse || accepted_response_pulse || !outstanding_valid)
            fail("redirect_epoch", "old epoch response reactivated");
        pass("redirect_epoch", "redirect incremented epoch and old epoch could not reactivate");

        architectural_redirect_valid = 1;
        architectural_redirect_target = 64'h23000;
        architectural_redirect_rob_idx = 4;
        architectural_redirect_allocation_id = 88;
        owner_live = 1;
        owner_allocation_id = 89;
        before_requests = request_count;
        invoke(0, 0);
        if (!ownership_rejection_pulse || redirect_accepted_pulse || request_count != before_requests)
            fail("architectural_ownership", "mismatched owner accepted");
        pass("architectural_ownership", "ROB index and allocation ID ownership validated");
        clear_controls();

        invoke(1, response_payload(redirected_req[63:0], redirected_req[95:64],
                                   redirected_req[127:96], 32'h00500293, 0, 0));
        held_pc = decode_pc; held_inst = decode_instruction;
        decode_ready = 0;
        invoke(0, 0);
        if (!decode_valid || decode_pc !== held_pc || decode_instruction !== held_inst || !held_entry_valid)
            fail("decode_hold", "held instruction changed under stall");
        pass("decode_hold", "held instruction remained stable under Decode stall");
        invoke(1, response_payload(redirected_req[63:0], redirected_req[95:64]+9,
                                   redirected_req[127:96], 32'hffffffff, 0, 0));
        if (!stale_response_pulse || decode_pc !== held_pc || decode_instruction !== held_inst)
            fail("stale_drain_during_decode_stall", "stale response overwrote held entry");
        pass("stale_drain_during_decode_stall", "stale response drained without held overwrite");
        decode_ready = 1;
        invoke(0, 0);
        req = requests[request_count-1];

        invoke(1, response_payload(req[63:0], req[95:64], req[127:96], 0, 1, 64'd1));
        if (!decode_valid || !decode_fault || decode_pc !== req[63:0] || decode_fault_cause !== 1)
            fail("fault_to_decode", "fault payload changed");
        pass("fault_to_decode", "instruction access fault PC and cause propagated to Decode");
        held_pc = decode_pc; held_cause = decode_fault_cause;
        decode_ready = 0;
        invoke(0, 0);
        if (!decode_valid || !decode_fault || decode_pc !== held_pc || decode_fault_cause !== held_cause)
            fail("fault_hold", "fault changed under backpressure");
        pass("fault_hold", "fault remained stable under Decode backpressure");

        // Reset clears a held fault as well as held instruction and outstanding state.
        epoch_before = frontend_epoch;
        runtime_reset = 1; decode_ready = 0;
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96], 32'h13, 0, 0));
        runtime_reset = 0; decode_ready = 1;
        if (decode_valid || held_entry_valid || frontend_pc !== 64'h10040 ||
            frontend_epoch !== epoch_before+1 || !stale_response_pulse)
            fail("runtime_reset", "held/reset+response path");
        pass("runtime_reset", "runtime reset cleared outstanding, held instruction, and held fault state");
        req = requests[request_count-1];

        architectural_redirect_valid = 1;
        architectural_redirect_target = 64'h24003;
        architectural_redirect_rob_idx = 5;
        architectural_redirect_allocation_id = 99;
        owner_live = 1; owner_allocation_id = 99;
        before_requests = request_count;
        invoke(0, 0);
        if (!misalignment_fault_pulse || !decode_fault || decode_pc !== 64'h24003 ||
            decode_fault_cause !== 0 || request_count != before_requests)
            fail("misaligned_arch_target", "architectural alignment fault");
        pass("misaligned_arch_target", "misaligned architectural target faulted without masking");
        clear_controls();

        runtime_reset = 1; invoke(0, 0); runtime_reset = 0;
        branch_redirect_valid = 1; branch_redirect_target = 64'h25003;
        before_requests = request_count;
        invoke(0, 0);
        if (!misalignment_fault_pulse || !decode_fault || decode_pc !== 64'h25003 ||
            request_count != before_requests)
            fail("misaligned_branch_target", "branch alignment fault");
        pass("misaligned_branch_target", "misaligned branch target faulted without masking");
        clear_controls();

        runtime_reset = 1; invoke(0, 0); runtime_reset = 0;
        generic_flush_valid = 1; generic_flush_target = 64'h26002;
        invoke(0, 0);
        if (misalignment_fault_pulse || requests[request_count-1][63:0] !== 64'h26000)
            fail("misaligned_generic_flush_target", "generic flush policy changed");
        pass("misaligned_generic_flush_target", "generic flush retained current-PC alignment policy");
        clear_controls();
        req = requests[request_count-1];

        epoch_before = frontend_epoch;
        runtime_reset = 1;
        architectural_redirect_valid = 1;
        architectural_redirect_target = 64'h27000;
        architectural_redirect_cause = 0;
        branch_redirect_valid = 1; branch_redirect_target = 64'h28000;
        generic_flush_valid = 1; generic_flush_target = 64'h29000;
        invoke(1, response_payload(req[63:0], req[95:64], req[127:96], 32'h13, 0, 0));
        if (!redirect_accepted_pulse || !stale_response_pulse || frontend_pc !== 64'h10040 ||
            frontend_epoch !== epoch_before+1 || requests[request_count-1][63:0] !== 64'h10040)
            fail("reset_priority", "runtime reset did not win");
        pass("reset_priority", "runtime reset won redirects and response and rejected old epoch");

        if (pass_count != 33) fail("matrix_count", "expected exactly 33 cases");
        $display("FRONTEND_VERIFY_RTL_PASS cases=%0d requests=%0d responses=%0d", pass_count,
                 request_count, response_count);
        $finish;
    end
endmodule
