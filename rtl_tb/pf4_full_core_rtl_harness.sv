`timescale 1ns/1ps

module pf4_full_core_rtl_harness (
    input wire clk, input wire rst_n, input wire start_fetch,
    input wire [1:0] fault_mode,
    input wire [191:0] seed_tdata, input wire seed_tvalid, output wire seed_tready,
    output wire tohost_seen, output wire [63:0] tohost_value,
    output wire tohost_commit_seen, output wire protocol_error,
    output wire [31:0] commit_count, output wire reset_completed,
    output wire brupdate_valid, output wire brupdate_mispredict,
    output wire exception_valid, output wire fault_site_requested,
    output wire fault_sent, output wire trap_vector_requested,
    output wire core_cycle_commit,
    output wire prediction_valid, output wire prediction_taken,
    output wire [63:0] prediction_token,
    output wire [31:0] brupdate_ftq_generation,
    output wire [5:0] brupdate_ftq_idx, output wire brupdate_ftq_lane,
    output wire [63:0] bim_attempts, output wire [63:0] bim_updates,
    output wire [63:0] bim_dropped, output wire [63:0] bim_stale_rejected,
    output wire commit_training_emit, output wire commit_training_taken,
    output wire [31:0] ftq_next_generation,
    output wire [7:0] ftq_count, output wire [4:0] rob_head,
    output wire [4:0] rob_tail, output wire rob_maybe_full
);
    wire [191:0] imem_req_tdata; wire imem_req_tvalid, imem_req_tready;
    wire [255:0] imem_resp_tdata; wire imem_resp_tvalid, imem_resp_tready;
    wire [319:0] dmem_req_tdata; wire dmem_req_tvalid, dmem_req_tready;
    wire [383:0] dmem_resp_tdata; wire dmem_resp_tvalid, dmem_resp_tready;
    wire [767:0] trace_tdata; wire trace_tvalid, trace_tready;
    wire imem_error, dmem_error, trace_error;
    wire [31:0] unused0, unused1, unused2, unused3, unused4, unused5, unused6;
    wire unused_pending0, unused_pending1, unused_branch;
    wire ap_local_block, ap_local_deadlock, io_success, io_halted, io_trap;
    wire io_cycle_valid; wire [63:0] io_cycle, io_instret;

    assign protocol_error = imem_error || dmem_error || trace_error;
    assign reset_completed = dut.reset_ctrl_completed;
    assign brupdate_valid = dut.state_brupdate_valid;
    assign brupdate_mispredict = dut.state_brupdate_mispredict;
    assign exception_valid = dut.state_exception_commit_valid;
    assign core_cycle_commit = dut.ap_CS_fsm_state34 && dut.reset_ctrl_completed;
    assign prediction_valid = dut.state_frontend_predictor_prediction_valid;
    assign prediction_taken = dut.state_frontend_predictor_predicted_taken;
    assign prediction_token = dut.state_frontend_prediction_token;
    assign brupdate_ftq_generation = dut.state_brupdate_uop_ftq_generation;
    assign brupdate_ftq_idx = dut.state_brupdate_uop_ftq_idx;
    assign brupdate_ftq_lane = dut.state_brupdate_uop_ftq_lane;
    assign bim_attempts = dut.state_bim_training_attempts;
    assign bim_updates = dut.state_bim_training_accepted;
    assign bim_dropped = dut.state_bim_training_dropped;
    assign bim_stale_rejected = dut.state_bim_training_stale_rejected;
    assign commit_training_emit =
        dut.grp_boom_core_cycle_io_fu_7725.grp_boom_core_step_fu_2189.
            grp_rob_commit_module_fu_3617_state_predictor_update_pending_valid_o_ap_vld &&
        dut.grp_boom_core_cycle_io_fu_7725.grp_boom_core_step_fu_2189.
            grp_rob_commit_module_fu_3617_state_predictor_update_pending_valid_o;
    assign commit_training_taken =
        dut.grp_boom_core_cycle_io_fu_7725.grp_boom_core_step_fu_2189.
            grp_rob_commit_module_fu_3617_state_predictor_update_pending_taken;
    assign ftq_next_generation = dut.state_ftq_next_generation_s;
    assign ftq_count = dut.state_ftq_count_s;
    assign rob_head = dut.state_rob_head;
    assign rob_tail = dut.state_rob_tail;
    assign rob_maybe_full = dut.state_rob_maybe_full;

    boom_core_pf4_rtl_top dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk), .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata), .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(imem_req_tready), .imem_resp_in_TDATA(imem_resp_tdata),
        .imem_resp_in_TVALID(imem_resp_tvalid), .imem_resp_in_TREADY(imem_resp_tready),
        .dmem_req_out_TDATA(dmem_req_tdata), .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(dmem_req_tready), .dmem_resp_in_TDATA(dmem_resp_tdata),
        .dmem_resp_in_TVALID(dmem_resp_tvalid), .dmem_resp_in_TREADY(dmem_resp_tready),
        .commit_trace_out_TDATA(trace_tdata), .commit_trace_out_TVALID(trace_tvalid),
        .commit_trace_out_TREADY(trace_tready), .test_seed_in_TDATA(seed_tdata),
        .test_seed_in_TVALID(seed_tvalid), .test_seed_in_TREADY(seed_tready),
        .io_success(io_success), .io_halted(io_halted), .io_trap(io_trap),
        .io_cycle_valid(io_cycle_valid), .io_cycle(io_cycle), .io_instret(io_instret));

    pf4_axis_imem_model imem_model (
        .clk(clk), .rst_n(rst_n), .start_fetch(start_fetch), .fault_mode(fault_mode),
        .req_tdata(imem_req_tdata), .req_tvalid(imem_req_tvalid), .req_tready(imem_req_tready),
        .resp_tdata(imem_resp_tdata), .resp_tvalid(imem_resp_tvalid), .resp_tready(imem_resp_tready),
        .request_count(unused0), .response_count(unused1), .protocol_error(imem_error),
        .fault_site_requested(fault_site_requested), .fault_sent(fault_sent),
        .trap_vector_requested(trap_vector_requested));
    axis_dmem_model dmem_model (
        .clk(clk), .rst_n(rst_n), .scenario_code(8'd0), .req_tdata(dmem_req_tdata),
        .req_tvalid(dmem_req_tvalid), .req_tready(dmem_req_tready),
        .resp_tdata(dmem_resp_tdata), .resp_tvalid(dmem_resp_tvalid),
        .resp_tready(dmem_resp_tready), .request_count(unused2), .response_count(unused3),
        .load_count(unused4), .store_count(unused5), .tohost_seen(tohost_seen),
        .tohost_value(tohost_value), .protocol_error(dmem_error), .pending_visible(unused_pending0));
    commit_trace_monitor trace_monitor (
        .clk(clk), .rst_n(rst_n), .scenario_code(8'd0),
        .imem_req_tdata(imem_req_tdata[127:0]), .imem_req_tvalid(imem_req_tvalid),
        .imem_req_tready(imem_req_tready), .dmem_req_tdata(dmem_req_tdata),
        .dmem_req_tvalid(dmem_req_tvalid), .dmem_req_tready(dmem_req_tready),
        .trace_tdata(trace_tdata), .trace_tvalid(trace_tvalid), .trace_tready(trace_tready),
        .commit_count(commit_count), .trace_transfer_count(unused6),
        .protocol_error(trace_error), .tohost_commit_seen(tohost_commit_seen),
        .branch_commit_visible(unused_branch));
endmodule
