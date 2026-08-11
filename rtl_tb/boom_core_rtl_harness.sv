`timescale 1ns/1ps

module boom_core_rtl_harness (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [7:0]  scenario_code,
    output wire        tohost_seen,
    output wire [63:0] tohost_value,
    output wire        protocol_error,
    output wire        imem_pending,
    output wire        dmem_pending,
    output wire        dmem_stalled,
    output wire        trace_stalled,
    output wire        branch_commit_visible,
    output wire        branch_recovery_visible,
    output wire        rob_nonempty,
    output wire        tohost_commit_seen,
    output wire [31:0] imem_transfers,
    output wire [31:0] dmem_transfers,
    output wire [31:0] dmem_loads,
    output wire [31:0] dmem_stores,
    output wire [31:0] commit_count,
    output wire [127:0] observed_imem_req,
    output wire        observed_imem_transfer
);
    wire [191:0] imem_req_tdata;
    wire imem_req_tvalid;
    wire imem_req_tready;
    wire [255:0] imem_resp_tdata;
    wire imem_resp_tvalid;
    wire imem_resp_tready;
    wire [319:0] dmem_req_tdata;
    wire dmem_req_tvalid;
    wire dmem_req_tready;
    wire [383:0] dmem_resp_tdata;
    wire dmem_resp_tvalid;
    wire dmem_resp_tready;
    wire [767:0] commit_trace_tdata;
    wire commit_trace_tvalid;
    wire commit_trace_tready;
    wire ap_local_block;
    wire ap_local_deadlock;
    wire imem_protocol_error;
    wire dmem_protocol_error;
    wire trace_protocol_error;
    wire [31:0] imem_responses;
    wire [31:0] dmem_responses;
    wire [31:0] trace_transfers;
    wire io_success;
    wire io_halted;
    wire io_trap;
    wire io_cycle_valid;
    wire [63:0] io_cycle;
    wire [63:0] io_instret;

    assign protocol_error = imem_protocol_error || dmem_protocol_error || trace_protocol_error;
    assign dmem_stalled = dmem_req_tvalid && !dmem_req_tready;
    assign trace_stalled = commit_trace_tvalid && !commit_trace_tready;
    assign observed_imem_req = imem_req_tdata[127:0];
    assign observed_imem_transfer = imem_req_tvalid && imem_req_tready;
    assign rob_nonempty =
        dut.grp_boom_core_cycle_io_fu_1764.state_rob_head !=
        dut.grp_boom_core_cycle_io_fu_1764.state_rob_tail;
    assign branch_recovery_visible =
        dut.grp_boom_core_cycle_io_fu_1764.state_brupdate_valid &&
        dut.grp_boom_core_cycle_io_fu_1764.state_brupdate_mispredict;

    boom_core_top dut (
        .ap_local_block(ap_local_block),
        .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk),
        .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata),
        .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(imem_req_tready),
        .imem_resp_in_TDATA(imem_resp_tdata),
        .imem_resp_in_TVALID(imem_resp_tvalid),
        .imem_resp_in_TREADY(imem_resp_tready),
        .dmem_req_out_TDATA(dmem_req_tdata),
        .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(dmem_req_tready),
        .dmem_resp_in_TDATA(dmem_resp_tdata),
        .dmem_resp_in_TVALID(dmem_resp_tvalid),
        .dmem_resp_in_TREADY(dmem_resp_tready),
        .commit_trace_out_TDATA(commit_trace_tdata),
        .commit_trace_out_TVALID(commit_trace_tvalid),
        .commit_trace_out_TREADY(commit_trace_tready),
        .io_success(io_success),
        .io_halted(io_halted),
        .io_trap(io_trap),
        .io_cycle_valid(io_cycle_valid),
        .io_cycle(io_cycle),
        .io_instret(io_instret)
    );

    axis_imem_model imem_model (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .req_tdata(imem_req_tdata), .req_tvalid(imem_req_tvalid), .req_tready(imem_req_tready),
        .resp_tdata(imem_resp_tdata), .resp_tvalid(imem_resp_tvalid), .resp_tready(imem_resp_tready),
        .request_count(imem_transfers), .response_count(imem_responses),
        .protocol_error(imem_protocol_error), .pending_visible(imem_pending)
    );

    axis_dmem_model dmem_model (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .req_tdata(dmem_req_tdata), .req_tvalid(dmem_req_tvalid), .req_tready(dmem_req_tready),
        .resp_tdata(dmem_resp_tdata), .resp_tvalid(dmem_resp_tvalid), .resp_tready(dmem_resp_tready),
        .request_count(dmem_transfers), .response_count(dmem_responses),
        .load_count(dmem_loads), .store_count(dmem_stores),
        .tohost_seen(tohost_seen), .tohost_value(tohost_value),
        .protocol_error(dmem_protocol_error), .pending_visible(dmem_pending)
    );

    commit_trace_monitor trace_monitor (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .imem_req_tdata(imem_req_tdata[127:0]), .imem_req_tvalid(imem_req_tvalid), .imem_req_tready(imem_req_tready),
        .dmem_req_tdata(dmem_req_tdata), .dmem_req_tvalid(dmem_req_tvalid), .dmem_req_tready(dmem_req_tready),
        .trace_tdata(commit_trace_tdata), .trace_tvalid(commit_trace_tvalid), .trace_tready(commit_trace_tready),
        .commit_count(commit_count), .trace_transfer_count(trace_transfers),
        .protocol_error(trace_protocol_error), .tohost_commit_seen(tohost_commit_seen),
        .branch_commit_visible(branch_commit_visible)
    );
endmodule
