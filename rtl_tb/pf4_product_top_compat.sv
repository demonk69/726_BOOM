`timescale 1ns/1ps

module boom_core_top (
    output wire ap_local_block, output wire ap_local_deadlock,
    input wire ap_clk, input wire ap_rst_n,
    output wire [191:0] imem_req_out_TDATA, output wire imem_req_out_TVALID,
    input wire imem_req_out_TREADY, input wire [255:0] imem_resp_in_TDATA,
    input wire imem_resp_in_TVALID, output wire imem_resp_in_TREADY,
    output wire [319:0] dmem_req_out_TDATA, output wire dmem_req_out_TVALID,
    input wire dmem_req_out_TREADY, input wire [383:0] dmem_resp_in_TDATA,
    input wire dmem_resp_in_TVALID, output wire dmem_resp_in_TREADY,
    output wire [767:0] commit_trace_out_TDATA, output wire commit_trace_out_TVALID,
    input wire commit_trace_out_TREADY, output wire io_success, output wire io_halted,
    output wire io_trap, output wire io_cycle_valid,
    output wire [63:0] io_cycle, output wire [63:0] io_instret
);
    wire seed_ready;
    wire [4:0] state_ftq_head_s = product_dut.state_ftq_head_s;
    wire [4:0] state_ftq_tail_s = product_dut.state_ftq_tail_s;
    wire [5:0] state_ftq_count_s = product_dut.state_ftq_count_s;
    wire [31:0] state_ftq_next_generation_s = product_dut.state_ftq_next_generation_s;
    wire state_ftq_retire_pending_valid = product_dut.state_ftq_retire_pending_valid;
    wire state_ftq_redirect_pending_valid = product_dut.state_ftq_redirect_pending_valid;
    wire state_exception_commit_valid = product_dut.state_exception_commit_valid;
    boom_core_pf4_rtl_top product_dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(ap_clk), .ap_rst_n(ap_rst_n),
        .imem_req_out_TDATA(imem_req_out_TDATA), .imem_req_out_TVALID(imem_req_out_TVALID),
        .imem_req_out_TREADY(imem_req_out_TREADY), .imem_resp_in_TDATA(imem_resp_in_TDATA),
        .imem_resp_in_TVALID(imem_resp_in_TVALID), .imem_resp_in_TREADY(imem_resp_in_TREADY),
        .dmem_req_out_TDATA(dmem_req_out_TDATA), .dmem_req_out_TVALID(dmem_req_out_TVALID),
        .dmem_req_out_TREADY(dmem_req_out_TREADY), .dmem_resp_in_TDATA(dmem_resp_in_TDATA),
        .dmem_resp_in_TVALID(dmem_resp_in_TVALID), .dmem_resp_in_TREADY(dmem_resp_in_TREADY),
        .commit_trace_out_TDATA(commit_trace_out_TDATA),
        .commit_trace_out_TVALID(commit_trace_out_TVALID),
        .commit_trace_out_TREADY(commit_trace_out_TREADY),
        .test_seed_in_TDATA(192'd0), .test_seed_in_TVALID(1'b0),
        .test_seed_in_TREADY(seed_ready), .io_success(io_success),
        .io_halted(io_halted), .io_trap(io_trap), .io_cycle_valid(io_cycle_valid),
        .io_cycle(io_cycle), .io_instret(io_instret));
endmodule
