`timescale 1ns/1ps

module fetch_packet_frontend_rtl_tb;
    reg ap_clk = 0, ap_rst = 1, ap_start = 0;
    reg imem_req_out_full_n = 1, imem_resp_in_empty_n = 0;
    reg [224:0] imem_resp_in_dout = 0;
    reg runtime_reset = 0, generic_flush = 0, decode_ready = 0;
    reg redirect_valid = 0;
    reg [63:0] redirect_target = 0;

    wire ap_done, ap_idle, ap_ready;
    wire imem_req_out_write, imem_resp_in_read;
    wire [128:0] imem_req_out_din;
    wire packet_valid;
    wire [7:0] packet_mask;
    wire [63:0] lane0_pc, lane1_pc, lane0_cause, lane1_cause;
    wire [31:0] lane0_instruction, lane1_instruction;
    wire lane0_exception, lane1_exception;
    wire carry_valid;
    wire [15:0] carry;
    wire [63:0] carry_pc;
    wire [7:0] occupancy;
    wire buffer_full;
    wire decode_valid, decode_valid_ap_vld;
    wire [63:0] decode_pc;
    wire [31:0] decode_instruction;
    wire outstanding_valid;
    wire [63:0] expected_address;
    wire [31:0] expected_fetch_id, expected_epoch;
    wire accepted_response_pulse, stale_response_pulse;

    integer pass_count = 0;
    integer request_count = 0;
    integer i;
    reg response_was_read = 0;
    reg [128:0] last_request = 0;
    reg fifo_order_ok;
    reg [63:0] saved_lane0_pc, saved_lane1_pc;
    reg [31:0] saved_lane0_instruction, saved_lane1_instruction;

    synth_fetch_packet_frontend_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din),
        .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write),
        .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n),
        .imem_resp_in_read(imem_resp_in_read),
        .runtime_reset(runtime_reset), .generic_flush(generic_flush),
        .decode_ready(decode_ready), .redirect_valid(redirect_valid),
        .redirect_target(redirect_target),
        .packet_valid(packet_valid), .packet_mask(packet_mask),
        .lane0_pc(lane0_pc), .lane0_instruction(lane0_instruction),
        .lane1_pc(lane1_pc), .lane1_instruction(lane1_instruction),
        .lane0_exception(lane0_exception), .lane0_cause(lane0_cause),
        .lane1_exception(lane1_exception), .lane1_cause(lane1_cause),
        .carry_valid(carry_valid), .carry(carry), .carry_pc(carry_pc),
        .occupancy(occupancy), .buffer_full(buffer_full),
        .decode_valid(decode_valid), .decode_valid_ap_vld(decode_valid_ap_vld),
        .decode_pc(decode_pc),
        .decode_instruction(decode_instruction),
        .outstanding_valid(outstanding_valid),
        .expected_address(expected_address),
        .expected_fetch_id(expected_fetch_id), .expected_epoch(expected_epoch),
        .accepted_response_pulse(accepted_response_pulse),
        .stale_response_pulse(stale_response_pulse)
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

    task fail(input [8*72-1:0] name, input [8*144-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "GATE5_3_B3I_PACKET_FRONTEND_RTL_FAIL case=%0s", name);
        end
    endtask

    task check_case(
        input [8*72-1:0] name,
        input [8*144-1:0] requirement,
        input condition
    );
        begin
            if (condition !== 1'b1) fail(name, requirement);
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task invoke;
        integer guard;
        begin
            @(negedge ap_clk); ap_start = 1;
            @(negedge ap_clk); ap_start = 0;
            guard = 0;
            while (ap_done !== 1'b1 && guard < 200) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1)
                fail("fe_generated_control_timeout", "ap_done did not arrive");
        end
    endtask

    function [224:0] response_payload;
        input [63:0] address;
        input [31:0] fetch_id;
        input [31:0] epoch;
        input [31:0] word;
        input fault;
        input [63:0] cause;
        response_payload = {cause, fault, word, epoch, fetch_id, address};
    endfunction

    task send_raw_response(
        input [63:0] address,
        input [31:0] fetch_id,
        input [31:0] epoch,
        input [31:0] word,
        input fault,
        input [63:0] cause
    );
        begin
            imem_resp_in_dout = response_payload(address, fetch_id, epoch,
                                                 word, fault, cause);
            imem_resp_in_empty_n = 1;
            response_was_read = 0;
            invoke();
            imem_resp_in_empty_n = 0;
            if (!response_was_read)
                fail("fe_response_stream_drain", "presented IMEM response was not read");
        end
    endtask

    task respond_match(
        input [31:0] word,
        input fault,
        input [63:0] cause
    );
        reg [63:0] address;
        reg [31:0] fetch_id;
        reg [31:0] epoch;
        begin
            if (!outstanding_valid)
                fail("fe_matching_response_without_request", "matching response requires an outstanding request");
            address = expected_address;
            fetch_id = expected_fetch_id;
            epoch = expected_epoch;
            send_raw_response(address, fetch_id, epoch, word, fault, cause);
        end
    endtask

    task redirect_to(input [63:0] target);
        begin
            redirect_target = target;
            redirect_valid = 1;
            invoke();
            redirect_valid = 0;
        end
    endtask

    task enqueue_pending;
        begin
            invoke();
            if (packet_valid)
                fail("fe_pending_enqueue", "pending packet did not enter available Fetch Buffer capacity");
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0;

        runtime_reset = 1;
        invoke();
        runtime_reset = 0;
        @(posedge ap_clk); #1;
        check_case("fe_reset_request_owned", "runtime initialization emits one owned IMEM request",
                   outstanding_valid && request_count > 0 &&
                   last_request[63:0] == expected_address &&
                    last_request[95:64] == expected_fetch_id &&
                    last_request[127:96] == expected_epoch);

        respond_match(32'h00850001, 0, 0);
        check_case("fe_cc_response_accepted", "matching C+C response is accepted exactly at current ownership", accepted_response_pulse && !stale_response_pulse);
        check_case("fe_cc_pending_mask3", "C+C creates a pending two-lane packet", packet_valid && packet_mask == 8'h03);
        check_case("fe_cc_pending_lane_pcs", "C+C pending lane PCs are ordered pc and pc+2", lane0_pc == 64'h10040 && lane1_pc == 64'h10042);
        check_case("fe_cc_pending_instructions", "C+C pending lanes expose both decompressed instructions", lane0_instruction == 32'h00000013 && lane1_instruction == 32'h00108093);
        enqueue_pending();
        check_case("fe_cc_atomic_enqueue", "C+C enters Fetch Buffer as two entries", occupancy == 2);
        decode_ready = 1;
        invoke();
        check_case("fe_cc_fifo_lane0", "Decode observes C+C lane0 first", decode_valid_ap_vld && decode_valid && decode_pc == 64'h10040 && decode_instruction == 32'h00000013 && occupancy == 1);
        invoke();
        check_case("fe_cc_fifo_lane1", "Decode observes C+C lane1 second", decode_valid && decode_pc == 64'h10042 && decode_instruction == 32'h00108093 && occupancy == 0);
        decode_ready = 0;

        redirect_to(64'h2000);
        respond_match(32'h00108093, 0, 0);
        check_case("fe_ordinary32_pending_mask1", "matching ordinary32 response creates mask1", accepted_response_pulse && packet_valid && packet_mask == 1);
        check_case("fe_ordinary32_pending_payload", "ordinary32 pending lane preserves PC and instruction", lane0_pc == 64'h2000 && lane0_instruction == 32'h00108093 && !lane0_exception);
        enqueue_pending();
        check_case("fe_ordinary32_single_enqueue", "ordinary32 adds exactly one Fetch Buffer entry", occupancy == 1);

        redirect_to(64'h3000);
        respond_match(32'h00930001, 0, 0);
        check_case("fe_c_partial_pending_mask1", "C plus partial ordinary32 exposes only complete C lane", packet_valid && packet_mask == 1 && lane0_pc == 64'h3000);
        check_case("fe_c_partial_carry", "C plus partial ordinary32 records carry at pc+2", carry_valid && carry == 16'h0093 && carry_pc == 64'h3002);
        enqueue_pending();
        check_case("fe_c_partial_no_partial_enqueue", "partial ordinary32 never occupies Fetch Buffer", occupancy == 1 && carry_valid && outstanding_valid && expected_address == 64'h3004);
        respond_match(32'h00850010, 0, 0);
        check_case("fe_carry_c_pending_mask3", "matching continuation completes carry plus upper C as mask3", packet_valid && packet_mask == 3 && !carry_valid);
        check_case("fe_carry_c_lane_pcs", "carry completion and upper C retain program-order PCs", lane0_pc == 64'h3002 && lane1_pc == 64'h3006);
        check_case("fe_carry_c_lane_payloads", "carry completion and C expansion are both observable", lane0_instruction == 32'h00100093 && lane1_instruction == 32'h00108093);
        enqueue_pending();
        check_case("fe_carry_c_atomic_enqueue", "carry plus C atomically adds two entries", occupancy == 3);

        redirect_to(64'h4000);
        for (i = 0; i < 4; i = i + 1) begin
            respond_match(32'h00850001, 0, 0);
            if (!(accepted_response_pulse && packet_valid && packet_mask == 3 &&
                  lane0_pc == 64'h4000 + i*4 && lane1_pc == 64'h4002 + i*4))
                fail("fe_fill_two_lane_response", "two-lane fill response did not match expected packet");
            enqueue_pending();
        end
        check_case("fe_fill_depth8_two_lane", "four matching two-lane responses fill depth8", occupancy == 8 && buffer_full);

        respond_match(32'h00850001, 0, 0);
        saved_lane0_pc = lane0_pc;
        saved_lane1_pc = lane1_pc;
        saved_lane0_instruction = lane0_instruction;
        saved_lane1_instruction = lane1_instruction;
        check_case("fe_full_two_lane_pending", "full Fetch Buffer retains newly built two-lane packet", packet_valid && packet_mask == 3 && occupancy == 8 && buffer_full && lane0_pc == 64'h4010 && lane1_pc == 64'h4012 && !lane0_exception && !lane1_exception && lane0_cause == 0 && lane1_cause == 0);
        invoke();
        check_case("fe_full_pending_stability", "full backpressure holds every exposed pending lane field stable", packet_valid && packet_mask == 3 && occupancy == 8 && lane0_pc == saved_lane0_pc && lane1_pc == saved_lane1_pc && lane0_instruction == saved_lane0_instruction && lane1_instruction == saved_lane1_instruction && !lane0_exception && !lane1_exception && lane0_cause == 0 && lane1_cause == 0);

        decode_ready = 1;
        invoke();
        check_case("fe_one_free_atomic_reject", "one freed slot rejects the complete two-lane pending packet", decode_valid && decode_pc == 64'h4000 && occupancy == 7 && packet_valid && packet_mask == 3);
        check_case("fe_one_free_no_partial_enqueue", "atomic rejection leaves every exposed pending lane field unchanged", lane0_pc == saved_lane0_pc && lane1_pc == saved_lane1_pc && lane0_instruction == saved_lane0_instruction && lane1_instruction == saved_lane1_instruction && !lane0_exception && !lane1_exception && lane0_cause == 0 && lane1_cause == 0);
        invoke();
        check_case("fe_pop_frees_second_slot_admit", "second simultaneous pop creates capacity and admits both pending lanes", decode_valid && decode_pc == 64'h4002 && occupancy == 8 && !packet_valid);

        fifo_order_ok = 1;
        for (i = 0; i < 8; i = i + 1) begin
            invoke();
            if (i < 6)
                fifo_order_ok = fifo_order_ok && decode_valid &&
                    decode_pc == 64'h4004 + i*2;
            else
                fifo_order_ok = fifo_order_ok && decode_valid &&
                    decode_pc == 64'h4010 + (i-6)*2;
        end
        check_case("fe_atomic_admission_fifo_order", "accepted two-lane packet follows all older entries in FIFO order", fifo_order_ok && occupancy == 0);
        decode_ready = 0;

        // Build buffer entries plus a pending C and carry, then verify each kill source.
        redirect_to(64'h5000);
        respond_match(32'h00850001, 0, 0);
        enqueue_pending();
        respond_match(32'h00930001, 0, 0);
        check_case("fe_redirect_kill_precondition", "redirect kill starts with buffer pending packet and carry", occupancy == 2 && packet_valid && packet_mask == 1 && carry_valid);
        redirect_to(64'h5100);
        check_case("fe_redirect_kills_frontend_state", "branch redirect kills pending packet Fetch Buffer and carry", !packet_valid && occupancy == 0 && !carry_valid && outstanding_valid && expected_address == 64'h5100);

        respond_match(32'h00850001, 0, 0);
        enqueue_pending();
        respond_match(32'h00930001, 0, 0);
        runtime_reset = 1;
        invoke();
        runtime_reset = 0;
        check_case("fe_runtime_reset_kills_frontend_state", "runtime reset kills pending packet Fetch Buffer and carry", !packet_valid && occupancy == 0 && !carry_valid && outstanding_valid);

        redirect_to(64'h7000);
        respond_match(32'h00850001, 0, 0);
        enqueue_pending();
        respond_match(32'h00930001, 0, 0);
        generic_flush = 1;
        invoke();
        generic_flush = 0;
        check_case("fe_generic_flush_kills_frontend_state", "generic flush kills pending packet Fetch Buffer and carry", !packet_valid && occupancy == 0 && !carry_valid && outstanding_valid);

        redirect_to(64'h8000);
        send_raw_response(expected_address, expected_fetch_id + 1,
                          expected_epoch, 32'h00850001, 0, 0);
        check_case("fe_stale_id_drained", "wrong fetch ID response is drained without acceptance", stale_response_pulse && !accepted_response_pulse && outstanding_valid && !packet_valid && occupancy == 0);
        send_raw_response(expected_address, expected_fetch_id,
                          expected_epoch + 1, 32'h00850001, 0, 0);
        check_case("fe_stale_epoch_drained", "wrong epoch response is drained without acceptance", stale_response_pulse && !accepted_response_pulse && outstanding_valid && !packet_valid && occupancy == 0);
        send_raw_response(expected_address + 4, expected_fetch_id,
                          expected_epoch, 32'h00850001, 0, 0);
        check_case("fe_stale_address_drained", "wrong address response is drained without acceptance", stale_response_pulse && !accepted_response_pulse && outstanding_valid && !packet_valid && occupancy == 0);
        respond_match(32'h00108093, 0, 0);
        check_case("fe_match_after_stale_drain", "correct owner response remains acceptable after stale drains", accepted_response_pulse && !stale_response_pulse && packet_valid && packet_mask == 1);

        redirect_to(64'h9000);
        imem_resp_in_dout = response_payload(expected_address, expected_fetch_id,
                                             expected_epoch, 32'h00850001, 0, 0);
        imem_resp_in_empty_n = 1;
        response_was_read = 0;
        redirect_target = 64'h9100;
        redirect_valid = 1;
        invoke();
        redirect_valid = 0;
        imem_resp_in_empty_n = 0;
        check_case("fe_redirect_over_response", "redirect wins while matching response is drained as stale", response_was_read && stale_response_pulse && !accepted_response_pulse && !packet_valid && occupancy == 0 && !carry_valid && outstanding_valid && expected_address == 64'h9100);

        respond_match(0, 1, 64'h000000000000000c);
        check_case("fe_fault_packet_pending", "matching access fault creates one pending fault lane", accepted_response_pulse && packet_valid && packet_mask == 1 && lane0_exception && lane0_pc == 64'h9100 && lane0_cause == 64'hc && !lane1_exception);
        redirect_to(64'h9200);
        check_case("fe_pending_fault_killed", "redirect kills pending fault before Fetch Buffer or Decode", !packet_valid && occupancy == 0 && !carry_valid && !decode_valid && outstanding_valid && expected_address == 64'h9200);

        if (pass_count < 30)
            fail("fe_integration_case_floor", "fewer than thirty observable integration cases executed");
        $display("GATE5_3_B3I_PACKET_FRONTEND_RTL_PASS cases=%0d", pass_count);
        $finish;
    end
endmodule
