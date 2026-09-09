`timescale 1ns/1ps

module pf3_ftq_atomic_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    wire ap_done;
    wire ap_ready;
    wire ap_idle;
    wire [128:0] imem_req_out_din;
    wire imem_req_out_write;
    wire imem_resp_in_read;

    reg runtime_reset, packet_valid;
    reg [7:0] packet_mask;
    reg [63:0] packet_base_pc;
    reg [31:0] lane0_instruction, lane1_instruction;
    reg lane0_rvc, lane1_rvc, prediction_valid, predicted_taken, target_valid;
    reg [63:0] predicted_target;
    reg [7:0] cfi_lane, cfi_type, predictor_metadata_index;
    reg [31:0] predictor_generation;
    reg fetch_buffer_blocked, retire_valid;
    reg [7:0] retire_ftq_idx, retire_lane;
    reg [31:0] retire_generation;
    reg redirect_valid;
    reg [7:0] redirect_owner_idx, redirect_surviving_mask;
    reg [31:0] redirect_owner_generation;
    reg lookup_valid;
    reg [7:0] lookup_idx;
    reg [31:0] lookup_generation;

    wire packet_accept, fetch_buffer_enqueue, ftq_alloc_ready, ftq_alloc_accepted;
    wire [7:0] reference_idx, final_mask, live_mask;
    wire [31:0] reference_generation;
    wire retire_accepted, redirect_accepted;
    wire [7:0] ftq_head, ftq_tail, ftq_count;
    wire ftq_full, reclaimed, lookup_hit;
    wire [63:0] lookup_base_pc, lookup_target;
    wire lookup_prediction_valid, lookup_predicted_taken, lookup_target_valid;
    wire [7:0] lookup_cfi_lane, lookup_cfi_type, lookup_metadata_index;
    wire [31:0] lookup_predictor_generation;

    always #5 ap_clk = ~ap_clk;

    synth_pf3_ftq_atomic_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_ready(ap_ready), .ap_idle(ap_idle),
        .imem_req_out_din(imem_req_out_din), .imem_req_out_full_n(1'b1),
        .imem_req_out_write(imem_req_out_write), .imem_resp_in_dout(225'b0),
        .imem_resp_in_empty_n(1'b0), .imem_resp_in_read(imem_resp_in_read),
        .runtime_reset(runtime_reset), .packet_valid(packet_valid),
        .packet_mask(packet_mask), .packet_base_pc(packet_base_pc),
        .lane0_instruction(lane0_instruction), .lane1_instruction(lane1_instruction),
        .lane0_rvc(lane0_rvc), .lane1_rvc(lane1_rvc),
        .prediction_valid(prediction_valid), .predicted_taken(predicted_taken),
        .target_valid(target_valid), .predicted_target(predicted_target),
        .cfi_lane(cfi_lane), .cfi_type(cfi_type),
        .predictor_metadata_index(predictor_metadata_index),
        .predictor_generation(predictor_generation),
        .fetch_buffer_blocked(fetch_buffer_blocked), .retire_valid(retire_valid),
        .retire_ftq_idx(retire_ftq_idx), .retire_generation(retire_generation),
        .retire_lane(retire_lane), .redirect_valid(redirect_valid),
        .redirect_owner_idx(redirect_owner_idx),
        .redirect_owner_generation(redirect_owner_generation),
        .redirect_surviving_mask(redirect_surviving_mask),
        .lookup_valid(lookup_valid), .lookup_idx(lookup_idx),
        .lookup_generation(lookup_generation), .packet_accept(packet_accept),
        .fetch_buffer_enqueue(fetch_buffer_enqueue),
        .ftq_alloc_ready(ftq_alloc_ready), .ftq_alloc_accepted(ftq_alloc_accepted),
        .reference_idx(reference_idx), .reference_generation(reference_generation),
        .final_mask(final_mask), .live_mask(live_mask),
        .retire_accepted(retire_accepted), .redirect_accepted(redirect_accepted),
        .ftq_head(ftq_head), .ftq_tail(ftq_tail), .ftq_count(ftq_count),
        .ftq_full(ftq_full), .reclaimed(reclaimed), .lookup_hit(lookup_hit),
        .lookup_base_pc(lookup_base_pc),
        .lookup_prediction_valid(lookup_prediction_valid),
        .lookup_predicted_taken(lookup_predicted_taken),
        .lookup_target_valid(lookup_target_valid), .lookup_target(lookup_target),
        .lookup_cfi_lane(lookup_cfi_lane), .lookup_cfi_type(lookup_cfi_type),
        .lookup_metadata_index(lookup_metadata_index),
        .lookup_predictor_generation(lookup_predictor_generation)
    );

    task clear_inputs;
    begin
        runtime_reset = 0; packet_valid = 0; packet_mask = 0;
        packet_base_pc = 0; lane0_instruction = 32'h00000013;
        lane1_instruction = 32'h00000013; lane0_rvc = 0; lane1_rvc = 0;
        prediction_valid = 0; predicted_taken = 0; target_valid = 0;
        predicted_target = 0; cfi_lane = 0; cfi_type = 0;
        predictor_metadata_index = 0; predictor_generation = 32'h41;
        fetch_buffer_blocked = 0; retire_valid = 0; retire_ftq_idx = 0;
        retire_generation = 0; retire_lane = 0; redirect_valid = 0;
        redirect_owner_idx = 0; redirect_owner_generation = 0;
        redirect_surviving_mask = 0; lookup_valid = 0; lookup_idx = 0;
        lookup_generation = 0;
    end
    endtask

    task invoke;
        integer timeout;
    begin
        @(posedge ap_clk); ap_start <= 1;
        @(posedge ap_clk); ap_start <= 0;
        timeout = 0;
        while (!ap_done && timeout < 512) begin
            @(posedge ap_clk); timeout = timeout + 1;
        end
        if (!ap_done) $fatal(1, "PF3_RTL_TIMEOUT");
        @(posedge ap_clk);
    end
    endtask

    integer i;
    integer checks = 0;
    reg [7:0] saved_idx;
    reg [31:0] saved_generation;
    initial begin
        clear_inputs();
        repeat (4) @(posedge ap_clk);
        ap_rst <= 0;
        repeat (2) @(posedge ap_clk);
        for (i = 0; i < 100; i = i + 1) begin
            clear_inputs(); runtime_reset = 1; invoke();
            if (ftq_count !== 0) $fatal(1, "reset_count case=%0d", i);
            checks = checks + 1;

            clear_inputs();
            packet_valid = 1;
            packet_mask = (i[0] ? 8'h03 : 8'h01);
            packet_base_pc = 64'h80000000 + i * 8;
            case (i % 4)
            0: begin lane0_instruction = 32'h0080006f; cfi_type = 2;
                     prediction_valid = 1; predicted_taken = 1;
                     target_valid = 1; predicted_target = packet_base_pc + 8; end
            1: begin lane0_instruction = 32'h00000463; cfi_type = 1;
                     prediction_valid = 1; predicted_taken = 1;
                     target_valid = 1; predicted_target = packet_base_pc + 8; end
            2: begin lane0_instruction = 32'h00008067; cfi_type = 3; end
            default: begin lane0_instruction = 32'h00000013; cfi_type = 0; end
            endcase
            predictor_metadata_index = packet_base_pc[8:1];
            invoke();
            if (!(packet_accept && fetch_buffer_enqueue && ftq_alloc_accepted))
                $fatal(1, "atomic admission case=%0d", i);
            if (final_mask !== packet_mask)
                $fatal(1, "final mask case=%0d", i);
            saved_idx = reference_idx;
            saved_generation = reference_generation;
            checks = checks + 3;

            clear_inputs(); lookup_valid = 1; lookup_idx = saved_idx;
            lookup_generation = saved_generation; invoke();
            if (!lookup_hit || lookup_base_pc !== (64'h80000000 + i * 8))
                $fatal(1, "lookup case=%0d", i);
            if ((i % 4) == 2 &&
                (lookup_prediction_valid || lookup_target_valid || lookup_target != 0))
                $fatal(1, "jalr metadata case=%0d", i);
            checks = checks + 2;

            clear_inputs(); retire_valid = 1; retire_ftq_idx = saved_idx;
            retire_generation = saved_generation; retire_lane = 0; invoke();
            if (!retire_accepted) $fatal(1, "retire case=%0d", i);
            checks = checks + 1;
        end
        $display("PF3_FTQ_ATOMIC_RTL_PASS cases=100 checks=%0d", checks);
        $finish;
    end
endmodule
