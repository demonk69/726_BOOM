`timescale 1ns/1ps

module pf4_branch_prediction_recovery_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    wire ap_done, ap_idle, ap_ready;

    reg [7:0] counter_state, resolution_cfi_type, stored_cfi_type;
    reg [7:0] lane, packet_mask;
    reg is_rvc, stale_ftq_generation, stale_predictor_generation;
    reg [63:0] pc, predicted_target;
    reg actual_taken;
    reg [63:0] actual_target;

    wire lookup_valid, cfi_match, prediction_valid, predicted_taken;
    wire predicted_target_valid;
    wire [63:0] lookup_target;
    wire [7:0] predictor_metadata_index;
    wire predictor_generation_match, resolved_actual_taken;
    wire [63:0] resolved_actual_target, resolved_fallthrough_pc;
    wire mispredict, stale_lookup, direction_mispredict, target_mispredict;
    wire [63:0] recovery_target;
    wire frontend_redirect_indication, rob_younger_killed;
    wire ftq_redirect_intent, ftq_squash_intent;
    wire [7:0] ftq_owner_idx;
    wire [31:0] ftq_owner_generation;
    wire [7:0] packet_surviving_mask;
    integer cases = 0;

    always #5 ap_clk = ~ap_clk;

    synth_pf4_branch_prediction_recovery_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .counter_state(counter_state), .resolution_cfi_type(resolution_cfi_type),
        .stored_cfi_type(stored_cfi_type), .lane(lane),
        .packet_mask(packet_mask), .is_rvc(is_rvc),
        .stale_ftq_generation(stale_ftq_generation),
        .stale_predictor_generation(stale_predictor_generation),
        .pc(pc), .predicted_target(predicted_target),
        .actual_taken(actual_taken), .actual_target(actual_target),
        .lookup_valid(lookup_valid), .cfi_match(cfi_match),
        .prediction_valid(prediction_valid), .predicted_taken(predicted_taken),
        .predicted_target_valid(predicted_target_valid),
        .lookup_target(lookup_target),
        .predictor_metadata_index(predictor_metadata_index),
        .predictor_generation_match(predictor_generation_match),
        .resolved_actual_taken(resolved_actual_taken),
        .resolved_actual_target(resolved_actual_target),
        .resolved_fallthrough_pc(resolved_fallthrough_pc),
        .mispredict(mispredict), .stale_lookup(stale_lookup),
        .direction_mispredict(direction_mispredict),
        .target_mispredict(target_mispredict),
        .recovery_target(recovery_target),
        .frontend_redirect_indication(frontend_redirect_indication),
        .rob_younger_killed(rob_younger_killed),
        .ftq_redirect_intent(ftq_redirect_intent),
        .ftq_squash_intent(ftq_squash_intent),
        .ftq_owner_idx(ftq_owner_idx),
        .ftq_owner_generation(ftq_owner_generation),
        .packet_surviving_mask(packet_surviving_mask)
    );

    task invoke;
        integer timeout;
    begin
        @(posedge ap_clk); ap_start <= 1;
        @(posedge ap_clk); ap_start <= 0;
        timeout = 0;
        while (!ap_done && timeout < 4096) begin
            @(posedge ap_clk);
            timeout = timeout + 1;
        end
        if (!ap_done) $fatal(1, "PF4_RTL_TIMEOUT case=%0d", cases);
        @(posedge ap_clk);
    end
    endtask

    task run_case;
        input [7:0] case_counter;
        input [7:0] case_cfi;
        input [7:0] case_stored_cfi;
        input case_lane;
        input case_rvc;
        input case_stale_ftq;
        input case_stale_predictor;
        input case_actual_taken;
        input case_target_mismatch;
        reg expected_lookup, expected_match, expected_prediction;
        reg expected_pred_taken, expected_target_valid;
        reg expected_direction, expected_target_mispredict, expected_mispredict;
        reg [63:0] expected_fallthrough, expected_recovery;
        reg [7:0] expected_mask;
    begin
        counter_state = case_counter;
        resolution_cfi_type = case_cfi;
        stored_cfi_type = case_stored_cfi;
        lane = case_lane;
        packet_mask = case_lane ? 8'h03 : (cases[0] ? 8'h03 : 8'h01);
        is_rvc = case_rvc;
        stale_ftq_generation = case_stale_ftq;
        stale_predictor_generation = case_stale_predictor;
        pc = 64'h0000_0000_8000_1000 + cases * 8 + (case_lane ?
             (case_rvc ? 2 : 4) : 0);
        predicted_target = 64'h0000_0000_8000_4000 + cases * 16;
        actual_taken = case_actual_taken;
        actual_target = predicted_target + (case_target_mismatch ? 4 : 0);

        expected_lookup = !case_stale_ftq;
        expected_match = expected_lookup && case_cfi == case_stored_cfi;
        expected_prediction = expected_match && case_stored_cfi != 3;
        expected_pred_taken = expected_prediction &&
            (case_stored_cfi == 2 || case_counter >= 2);
        expected_target_valid = expected_pred_taken;
        expected_direction = expected_prediction &&
            expected_pred_taken != case_actual_taken;
        expected_target_mispredict = expected_prediction && expected_pred_taken &&
            case_actual_taken && case_target_mismatch;
        expected_mispredict = !expected_lookup || !expected_match || case_cfi == 3 ||
            !expected_prediction || expected_direction || expected_target_mispredict;
        expected_fallthrough = pc + (case_rvc ? 2 : 4);
        expected_recovery = case_actual_taken ? actual_target : expected_fallthrough;
        expected_mask = case_lane ? 3 : 1;

        invoke();

        if (lookup_valid !== expected_lookup || cfi_match !== expected_match)
            $fatal(1, "PF4_LOOKUP case=%0d", cases);
        if (prediction_valid !== expected_prediction ||
            predicted_taken !== expected_pred_taken ||
            predicted_target_valid !== expected_target_valid)
            $fatal(1, "PF4_PREDICTION case=%0d", cases);
        if (expected_target_valid && lookup_target !== predicted_target)
            $fatal(1, "PF4_LOOKUP_TARGET case=%0d", cases);
        if (predictor_metadata_index !== (expected_match ? pc[6:1] : 0))
            $fatal(1, "PF4_METADATA case=%0d", cases);
        if (predictor_generation_match !==
            (expected_match && !case_stale_predictor))
            $fatal(1, "PF4_PREDICTOR_GENERATION case=%0d", cases);
        if (resolved_actual_taken !== case_actual_taken ||
            resolved_actual_target !== actual_target ||
            resolved_fallthrough_pc !== expected_fallthrough)
            $fatal(1, "PF4_ACTUAL case=%0d", cases);
        if (mispredict !== expected_mispredict ||
            stale_lookup !== (!expected_match) ||
            direction_mispredict !== expected_direction ||
            target_mispredict !== expected_target_mispredict)
            $fatal(1, "PF4_CLASS case=%0d", cases);
        if (recovery_target !== expected_recovery ||
            frontend_redirect_indication !== expected_mispredict)
            $fatal(1, "PF4_RECOVERY_TARGET case=%0d", cases);
        if (rob_younger_killed !== expected_mispredict ||
            ftq_redirect_intent !== expected_mispredict ||
            ftq_squash_intent !== expected_mispredict)
            $fatal(1, "PF4_KILL_INTENT case=%0d", cases);
        if (expected_mispredict &&
            (ftq_owner_idx !== 0 || ftq_owner_generation !==
             (case_stale_ftq ? 2 : 1) || packet_surviving_mask !== expected_mask))
            $fatal(1, "PF4_FTQ_IDENTITY case=%0d", cases);
        cases = cases + 1;
    end
    endtask

    integer i;
    initial begin
        counter_state = 0; resolution_cfi_type = 0; stored_cfi_type = 0;
        lane = 0; packet_mask = 1; is_rvc = 0; stale_ftq_generation = 0;
        stale_predictor_generation = 0; pc = 0; predicted_target = 0;
        actual_taken = 0; actual_target = 0;
        repeat (4) @(posedge ap_clk);
        ap_rst <= 0;
        repeat (2) @(posedge ap_clk);

        // 128 conditional cases: all BIM states x actual direction x stale FTQ
        // x CFI match x instruction length x lane identity.
        for (i = 0; i < 128; i = i + 1) begin
            run_case(i[1:0], 1, i[4] ? 2 : 1, i[6], i[5], i[3], i[1],
                     i[2], i[5]);
        end

        // Six JAL and six JALR cases complete the exact mandatory set.
        for (i = 0; i < 6; i = i + 1)
            run_case(i[1:0], 2, 2, i[0], i[1], 0, i[2], i[1], i[2]);
        for (i = 0; i < 6; i = i + 1)
            run_case(i[1:0], 3, 3, i[0], i[1], 0, i[2], i[1], i[2]);

        if (cases != 140) $fatal(1, "PF4_CASE_COUNT observed=%0d", cases);
        $display("PF4_BRANCH_PREDICTION_RECOVERY_RTL_PASS cases=140");
        $finish;
    end
endmodule
