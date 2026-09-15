`timescale 1ns/1ps

module pf4_full_core_rtl_tb;
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;
    reg clk, rst_n, start_fetch, seed_tvalid;
    reg [1:0] fault_mode;
    reg [191:0] seed_tdata;
    wire seed_tready, tohost_seen, tohost_commit_seen, protocol_error;
    wire [63:0] tohost_value;
    wire [31:0] commit_count;
    wire reset_completed, brupdate_valid, brupdate_mispredict, exception_valid;
    wire fault_site_requested, fault_sent, trap_vector_requested;
    wire core_cycle_commit, prediction_valid, prediction_taken;
    wire [63:0] prediction_token;
    wire [31:0] brupdate_ftq_generation;
    wire [5:0] brupdate_ftq_idx;
    wire brupdate_ftq_lane;
    wire [63:0] bim_attempts, bim_updates, bim_dropped, bim_stale_rejected;
    wire commit_training_emit, commit_training_taken;
    wire [31:0] ftq_next_generation;
    wire [7:0] ftq_count;
    wire [4:0] rob_head, rob_tail;
    wire rob_maybe_full;
    integer cycles, max_cycles, init_file, scan_status, init_count, i, repeats;
    integer branch_updates, mispredicts, exceptions;
    integer conditional_predictions, predicted_taken, predicted_not_taken;
    integer bim_taken_updates, bim_not_taken_updates;
    integer max_rob_occupancy, max_ftq_occupancy, rob_occupancy;
    reg [63:0] bim_attempts_baseline, bim_updates_baseline;
    reg [63:0] bim_dropped_baseline, bim_stale_baseline;
    reg previous_brupdate_valid, previous_exception_valid, prediction_seen;
    reg [38:0] previous_brupdate_identity;
    reg [63:0] previous_prediction_token;
    integer init_counter [0:31];
    reg [63:0] init_offset [0:31];
    reg [63:0] seed_pc;
    string program_name, init_path;

    pf4_full_core_rtl_harness harness (
        .clk(clk), .rst_n(rst_n), .start_fetch(start_fetch), .fault_mode(fault_mode),
        .seed_tdata(seed_tdata), .seed_tvalid(seed_tvalid), .seed_tready(seed_tready),
        .tohost_seen(tohost_seen), .tohost_value(tohost_value),
        .tohost_commit_seen(tohost_commit_seen), .protocol_error(protocol_error),
        .commit_count(commit_count), .reset_completed(reset_completed),
        .brupdate_valid(brupdate_valid), .brupdate_mispredict(brupdate_mispredict),
        .exception_valid(exception_valid), .fault_site_requested(fault_site_requested),
        .fault_sent(fault_sent), .trap_vector_requested(trap_vector_requested),
        .core_cycle_commit(core_cycle_commit), .prediction_valid(prediction_valid),
        .prediction_taken(prediction_taken),
        .prediction_token(prediction_token),
        .brupdate_ftq_generation(brupdate_ftq_generation),
        .brupdate_ftq_idx(brupdate_ftq_idx), .brupdate_ftq_lane(brupdate_ftq_lane),
        .bim_attempts(bim_attempts), .bim_updates(bim_updates),
        .bim_dropped(bim_dropped), .bim_stale_rejected(bim_stale_rejected),
        .commit_training_emit(commit_training_emit),
        .commit_training_taken(commit_training_taken),
        .ftq_next_generation(ftq_next_generation), .ftq_count(ftq_count),
        .rob_head(rob_head), .rob_tail(rob_tail), .rob_maybe_full(rob_maybe_full));

    initial begin clk = 0; forever #5 clk = ~clk; end
    always @(posedge clk) if (rst_n && start_fetch && commit_training_emit) begin
        if (commit_training_taken) bim_taken_updates = bim_taken_updates + 1;
        else bim_not_taken_updates = bim_not_taken_updates + 1;
    end
    always @(posedge clk) if (rst_n && core_cycle_commit) begin
        #1;
        if (brupdate_valid && (!previous_brupdate_valid ||
            previous_brupdate_identity != {brupdate_ftq_generation, brupdate_ftq_idx,
                                           brupdate_ftq_lane})) begin
            branch_updates = branch_updates + 1;
            if (brupdate_mispredict) mispredicts = mispredicts + 1;
        end
        previous_brupdate_valid = brupdate_valid;
        previous_brupdate_identity = {brupdate_ftq_generation, brupdate_ftq_idx,
                                      brupdate_ftq_lane};
        if (exception_valid && !previous_exception_valid) exceptions = exceptions + 1;
        previous_exception_valid = exception_valid;
        if (prediction_valid && (!prediction_seen ||
            prediction_token != previous_prediction_token)) begin
            conditional_predictions = conditional_predictions + 1;
            if (prediction_taken) predicted_taken = predicted_taken + 1;
            else predicted_not_taken = predicted_not_taken + 1;
            prediction_seen = 1;
            previous_prediction_token = prediction_token;
        end
        if (ftq_count > max_ftq_occupancy) max_ftq_occupancy = ftq_count;
        if (rob_maybe_full && rob_head == rob_tail) rob_occupancy = 32;
        else if (rob_tail >= rob_head) rob_occupancy = rob_tail - rob_head;
        else rob_occupancy = 32 + rob_tail - rob_head;
        if (rob_occupancy > max_rob_occupancy) max_rob_occupancy = rob_occupancy;
    end

    task automatic send_seed(input [63:0] pc, input bit taken);
        begin
            seed_tdata = 0;
            seed_tdata[127:64] = pc;
            seed_tdata[144] = taken;
            seed_tvalid = 1;
            do @(posedge clk); while (!seed_tready);
            @(negedge clk); seed_tvalid = 0;
        end
    endtask

    initial begin
        if (!$value$plusargs("PROGRAM_NAME=%s", program_name)) program_name = "pf4_pred_nt_actual_nt";
        if (!$value$plusargs("INIT=%s", init_path)) init_path = "/tmp/boom_hls/pf4/programs/empty.init";
        if (!$value$plusargs("FAULT_MODE=%d", fault_mode)) fault_mode = 0;
        if (!$value$plusargs("MAX_CYCLES=%d", max_cycles)) max_cycles = 1000000;
        init_count = 0;
        init_file = $fopen(init_path, "r");
        if (init_file != 0) begin
            while (!$feof(init_file) && init_count < 32) begin
                scan_status = $fscanf(init_file, "%h %d\n", init_offset[init_count], init_counter[init_count]);
                if (scan_status == 2) init_count = init_count + 1;
            end
            $fclose(init_file);
        end
        rst_n = 0; start_fetch = 0; seed_tvalid = 0; seed_tdata = 0;
        cycles = 0; branch_updates = 0; mispredicts = 0; exceptions = 0;
        conditional_predictions = 0; predicted_taken = 0; predicted_not_taken = 0;
        bim_taken_updates = 0; bim_not_taken_updates = 0;
        bim_attempts_baseline = 0; bim_updates_baseline = 0;
        bim_dropped_baseline = 0; bim_stale_baseline = 0;
        previous_brupdate_valid = 0; previous_exception_valid = 0; prediction_seen = 0;
        previous_brupdate_identity = 0; previous_prediction_token = 0;
        max_rob_occupancy = 0; max_ftq_occupancy = 0; rob_occupancy = 0;
        repeat (5) @(negedge clk); rst_n = 1;
        wait (reset_completed);
        for (i = 0; i < init_count; i = i + 1) begin
            seed_pc = RESET_VECTOR + init_offset[i];
            repeats = init_counter[i] == 3 ? 2 : (init_counter[i] == 2 ? 1 : 0);
            repeat (repeats) send_seed(seed_pc, 1'b1);
        end
        @(posedge core_cycle_commit);
        @(negedge clk);
        bim_attempts_baseline = bim_attempts;
        bim_updates_baseline = bim_updates;
        bim_dropped_baseline = bim_dropped;
        bim_stale_baseline = bim_stale_rejected;
        start_fetch = 1;
        while (cycles < max_cycles &&
               !(fault_mode == 2 && (program_name == "pf4_pred_t_actual_nt_fault" ||
                                     program_name == "pf5_pred_t_actual_nt_fault") ?
                 (fault_sent && exceptions != 0) : (tohost_seen && tohost_commit_seen))) begin
            @(posedge clk); cycles = cycles + 1;
        end
        @(posedge clk);
        if (cycles >= max_cycles || protocol_error)
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL program=%s timeout/protocol", program_name);
        if (!(fault_mode == 2 && (program_name == "pf4_pred_t_actual_nt_fault" ||
                                  program_name == "pf5_pred_t_actual_nt_fault")) &&
            (tohost_value != 1 || !tohost_commit_seen))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL program=%s completion", program_name);
        if ((program_name == "pf4_pred_t_actual_t" ||
             program_name == "pf5_pred_t_actual_t") &&
            (fault_site_requested || fault_sent || exceptions != 0))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL predicted-T actual-T exposed younger fault");
        if ((program_name == "pf4_pred_t_actual_nt_fault" ||
             program_name == "pf5_pred_t_actual_nt_fault") &&
            (!fault_site_requested || !fault_sent || exceptions == 0 || mispredicts == 0))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL predicted-T actual-NT did not refetch fault");
        if ((program_name == "pf4_fault_refetch" ||
             program_name == "pf5_fault_no_training") &&
            (!fault_sent || exceptions == 0 || !trap_vector_requested))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL fault refetch contract");
`ifdef PF5_TRAINING_EXPECTED
        repeats = 0;
        for (i = 0; i < init_count; i = i + 1) begin
            if (harness.dut.state_predictor_valid_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] !==
                    (init_counter[i] >= 2) ||
                (harness.dut.state_predictor_valid_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] === 1'b1 &&
                 harness.dut.state_predictor_counters_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] !== init_counter[i]))
                repeats = repeats + 1;
        end
        if ((program_name == "pf5_pred_nt_actual_nt" ||
             program_name == "pf5_pred_nt_actual_t" ||
             program_name == "pf5_pred_t_actual_nt" ||
             program_name == "pf5_rvc_commit" ||
             program_name == "pf5_same_packet_kill" ||
             program_name == "pf5_ftq_wrap_training" ||
             program_name == "pf5_mixed_long_training") && repeats == 0)
            $fatal(1, "PF5_FULL_CORE_RTL_FAIL no BIM training effect program=%s", program_name);
        if ((program_name == "pf4_pred_nt_actual_nt" ||
             program_name == "pf4_pred_nt_actual_t" ||
             program_name == "pf4_pred_t_actual_nt" ||
             program_name == "pf4_rvc_mispredict" ||
             program_name == "pf4_same_packet_kill" ||
             program_name == "pf4_ftq_wrap_recovery" ||
             program_name == "pf4_mixed_long_control") && repeats == 0)
            $fatal(1, "PF5_FULL_CORE_RTL_FAIL no BIM training effect program=%s", program_name);
`else
        for (i = 0; i < init_count; i = i + 1) begin
            if (init_counter[i] >= 2 &&
                (harness.dut.state_predictor_valid_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] !== 1'b1 ||
                 harness.dut.state_predictor_counters_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] !== init_counter[i]))
                $fatal(1, "PF4_FULL_CORE_RTL_FAIL BIM changed program=%s entry=%0d", program_name, i);
        end
`endif
        harness.trace_monitor.finish_trace("pass");
`ifdef PF5_TRAINING_EXPECTED
        if (bim_taken_updates + bim_not_taken_updates !=
            bim_updates - bim_updates_baseline)
            $fatal(1, "PF6_RTL_COVERAGE BIM direction accounting mismatch");
        $display("PF5_FULL_CORE_RTL_PASS program=%s branches=%0d mispredicts=%0d exceptions=%0d commits=%0d cycles=%0d bim_entries=%0d",
                  program_name, branch_updates, mispredicts, exceptions, commit_count, cycles, init_count);
        $display("PF6_RTL_COVERAGE program=%s conditional_predictions=%0d predicted_taken=%0d predicted_not_taken=%0d correct_predictions=%0d mispredict_redirects=%0d BIM_attempts=%0d BIM_updates=%0d BIM_taken_updates=%0d BIM_not_taken_updates=%0d BIM_dropped=%0d BIM_stale_rejected=%0d FTQ_allocations=%0d FTQ_reclaims=%0d FTQ_squashes=%0d FTQ_wraps=%0d FTQ_slot_reuses=%0d exceptions=%0d fault_refetches=%0d ROB_commits=%0d max_rob_occupancy=%0d max_ftq_occupancy=%0d",
                 program_name, conditional_predictions, predicted_taken, predicted_not_taken,
                 conditional_predictions - mispredicts, mispredicts,
                 bim_attempts - bim_attempts_baseline, bim_updates - bim_updates_baseline,
                 bim_taken_updates, bim_not_taken_updates, bim_dropped - bim_dropped_baseline,
                 bim_stale_rejected - bim_stale_baseline,
                 ftq_next_generation - 1,
                 ftq_next_generation - 1 - ftq_count, mispredicts,
                 (ftq_next_generation - 1) / 32,
                 ftq_next_generation > 33 ? ftq_next_generation - 33 : 0,
                 exceptions, fault_sent ? 1 : 0, commit_count,
                 max_rob_occupancy, max_ftq_occupancy);
`else
        $display("PF4_FULL_CORE_RTL_PASS program=%s branches=%0d mispredicts=%0d exceptions=%0d commits=%0d cycles=%0d bim_entries=%0d",
                 program_name, branch_updates, mispredicts, exceptions, commit_count, cycles, init_count);
`endif
        $finish;
    end
endmodule
