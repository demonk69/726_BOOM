`timescale 1ns/1ps

module pf5_commit_bim_training_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    wire ap_done, ap_idle, ap_ready;
    reg [7:0] counter_state, cfi_type;
    reg actual_taken, resolved, exception;
    reg stale_ftq_generation, stale_predictor_generation;
    reg metadata_mismatch, request_same_index;
    reg [63:0] pc;
    wire rob_commit_valid, training_update_valid;
    wire [7:0] training_bim_index;
    wire training_actual_taken, predictor_generation_match;
    wire prediction_after_valid, prediction_after_taken;
    wire retire_valid, retire_accepted, ftq_reference_before;
    wire ftq_reference_after, ftq_reclaimed;
    wire rob_commit_valid_ap_vld, training_update_valid_ap_vld;
    wire training_bim_index_ap_vld, training_actual_taken_ap_vld;
    wire predictor_generation_match_ap_vld, prediction_after_valid_ap_vld;
    wire prediction_after_taken_ap_vld, retire_valid_ap_vld;
    wire retire_accepted_ap_vld, ftq_reference_before_ap_vld;
    wire ftq_reference_after_ap_vld, ftq_reclaimed_ap_vld;
    reg rob_commit_valid_q, training_update_valid_q;
    reg [7:0] training_bim_index_q;
    reg training_actual_taken_q, predictor_generation_match_q;
    reg prediction_after_valid_q, prediction_after_taken_q;
    reg retire_valid_q, retire_accepted_q, ftq_reference_before_q;
    reg ftq_reference_after_q, ftq_reclaimed_q;
    integer cases = 0;

    always #5 ap_clk = ~ap_clk;

    synth_pf5_commit_bim_training_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .counter_state(counter_state), .cfi_type(cfi_type),
        .actual_taken(actual_taken), .resolved(resolved), .exception(exception),
        .stale_ftq_generation(stale_ftq_generation),
        .stale_predictor_generation(stale_predictor_generation),
        .metadata_mismatch(metadata_mismatch),
        .request_same_index(request_same_index), .pc(pc),
        .rob_commit_valid(rob_commit_valid),
        .rob_commit_valid_ap_vld(rob_commit_valid_ap_vld),
        .training_update_valid(training_update_valid),
        .training_update_valid_ap_vld(training_update_valid_ap_vld),
        .training_bim_index(training_bim_index),
        .training_bim_index_ap_vld(training_bim_index_ap_vld),
        .training_actual_taken(training_actual_taken),
        .training_actual_taken_ap_vld(training_actual_taken_ap_vld),
        .predictor_generation_match(predictor_generation_match),
        .predictor_generation_match_ap_vld(predictor_generation_match_ap_vld),
        .prediction_after_valid(prediction_after_valid),
        .prediction_after_valid_ap_vld(prediction_after_valid_ap_vld),
        .prediction_after_taken(prediction_after_taken),
        .prediction_after_taken_ap_vld(prediction_after_taken_ap_vld),
        .retire_valid(retire_valid), .retire_valid_ap_vld(retire_valid_ap_vld),
        .retire_accepted(retire_accepted),
        .retire_accepted_ap_vld(retire_accepted_ap_vld),
        .ftq_reference_before(ftq_reference_before),
        .ftq_reference_before_ap_vld(ftq_reference_before_ap_vld),
        .ftq_reference_after(ftq_reference_after),
        .ftq_reference_after_ap_vld(ftq_reference_after_ap_vld),
        .ftq_reclaimed(ftq_reclaimed),
        .ftq_reclaimed_ap_vld(ftq_reclaimed_ap_vld)
    );

    always @(posedge ap_clk) begin
        if (rob_commit_valid_ap_vld) rob_commit_valid_q <= rob_commit_valid;
        if (training_update_valid_ap_vld) training_update_valid_q <= training_update_valid;
        if (training_bim_index_ap_vld) training_bim_index_q <= training_bim_index;
        if (training_actual_taken_ap_vld) training_actual_taken_q <= training_actual_taken;
        if (predictor_generation_match_ap_vld)
            predictor_generation_match_q <= predictor_generation_match;
        if (prediction_after_valid_ap_vld) prediction_after_valid_q <= prediction_after_valid;
        if (prediction_after_taken_ap_vld) prediction_after_taken_q <= prediction_after_taken;
        if (retire_valid_ap_vld) retire_valid_q <= retire_valid;
        if (retire_accepted_ap_vld) retire_accepted_q <= retire_accepted;
        if (ftq_reference_before_ap_vld) ftq_reference_before_q <= ftq_reference_before;
        if (ftq_reference_after_ap_vld) ftq_reference_after_q <= ftq_reference_after;
        if (ftq_reclaimed_ap_vld) ftq_reclaimed_q <= ftq_reclaimed;
    end

    task invoke;
        integer timeout;
    begin
        @(posedge ap_clk); ap_start <= 1;
        @(posedge ap_clk); ap_start <= 0;
        timeout = 0;
        while (!ap_done && timeout < 8192) begin
            @(posedge ap_clk);
            timeout = timeout + 1;
        end
        if (!ap_done) $fatal(1, "PF5_RTL_TIMEOUT case=%0d", cases);
        #1;
    end
    endtask

    task run_case;
        input [7:0] selected_cfi;
        input [1:0] selected_counter;
        input selected_taken, selected_resolved, selected_exception;
        input selected_stale_ftq, selected_stale_predictor;
        input selected_metadata_mismatch, selected_same_index;
        reg expected_train, expected_retire, expected_after_taken;
        reg [2:0] updated_counter;
    begin
        counter_state = selected_counter;
        cfi_type = selected_cfi;
        actual_taken = selected_taken;
        resolved = selected_resolved;
        exception = selected_exception;
        stale_ftq_generation = selected_stale_ftq;
        stale_predictor_generation = selected_stale_predictor;
        metadata_mismatch = selected_metadata_mismatch;
        request_same_index = selected_same_index;
        pc = 64'h0000_0000_8000_2000 + cases * 16;
        expected_train = selected_cfi == 1 && selected_resolved &&
            !selected_exception && !selected_stale_ftq &&
            !selected_stale_predictor && !selected_metadata_mismatch;
        expected_retire = !selected_exception;
        if (expected_train) begin
            if (selected_taken)
                updated_counter = selected_counter == 3 ? 3 : selected_counter + 1;
            else
                updated_counter = selected_counter == 0 ? 0 : selected_counter - 1;
        end else updated_counter = selected_counter;
        expected_after_taken = selected_same_index && updated_counter >= 2;

        invoke();
        if (rob_commit_valid_q !== !selected_exception)
            $fatal(1, "PF5_COMMIT case=%0d observed=%b expected=%b exception=%b",
                   cases, rob_commit_valid_q, !selected_exception,
                   selected_exception);
        if (training_update_valid_q !== expected_train)
            $fatal(1, "PF5_ELIGIBILITY case=%0d", cases);
        if (expected_train && (training_bim_index_q !== pc[8:1] ||
            training_actual_taken_q !== selected_taken ||
            predictor_generation_match_q !== 1'b1))
            $fatal(1, "PF5_UPDATE_PAYLOAD case=%0d", cases);
        if (prediction_after_valid_q !== 1'b1 ||
            prediction_after_taken_q !== expected_after_taken)
            $fatal(1, "PF5_PREDICTION_AFTER case=%0d", cases);
        if (retire_valid_q !== expected_retire ||
            retire_accepted_q !== (expected_retire && !selected_stale_ftq))
            $fatal(1, "PF5_RETIRE case=%0d", cases);
        if (ftq_reference_before_q !== !selected_stale_ftq)
            $fatal(1, "PF5_REFERENCE_BEFORE case=%0d", cases);
        if (ftq_reclaimed_q !== (expected_retire && !selected_stale_ftq) ||
            ftq_reference_after_q !== (selected_exception || selected_stale_ftq))
            $fatal(1, "PF5_ORDER case=%0d", cases);
        cases = cases + 1;
    end
    endtask

    integer i;
    reg [7:0] selected_cfi;
    initial begin
        counter_state = 0; cfi_type = 0; actual_taken = 0; resolved = 0;
        exception = 0; stale_ftq_generation = 0;
        stale_predictor_generation = 0; metadata_mismatch = 0;
        request_same_index = 0; pc = 0;
        repeat (4) @(posedge ap_clk);
        ap_rst <= 0;
        repeat (2) @(posedge ap_clk);

        for (i = 0; i < 160; i = i + 1) begin
            selected_cfi = i < 128 ? 1 : (i < 144 ? 2 : 3);
            run_case(selected_cfi, i[1:0], i[2], i[3], i[4], i[5],
                     i[6], i[5] & i[2], !i[0]);
        end
        if (cases != 160) $fatal(1, "PF5_CASE_COUNT observed=%0d", cases);
        $display("PF5_COMMIT_BIM_TRAINING_RTL_PASS cases=160");
        $finish;
    end
endmodule
