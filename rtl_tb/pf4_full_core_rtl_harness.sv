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
    output wire fault_sent, output wire trap_vector_requested
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
