`timescale 1 ns / 1 ps

module boom_l3_ppa4_system_top (
    input wire ap_clk,
    input wire ap_rst_n,
    output wire io_success,
    output wire io_halted,
    output wire io_trap,
    output wire io_cycle_valid,
    output wire [63:0] io_cycle,
    output wire [63:0] io_instret,
    output wire [31:0] ppa4_imem_request_count,
    output wire [31:0] ppa4_dmem_read_count,
    output wire [31:0] ppa4_dmem_write_count,
    output wire [31:0] ppa4_commit_trace_count,
    output wire ppa4_imem_fault,
    output wire ppa4_dmem_fault,
    output wire ppa4_commit_trace_overflow
);

    wire ap_local_block;
    wire ap_local_deadlock;

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
    wire [31:0] dmem_response_count_unused;
    wire [31:0] commit_trace_checksum_unused;

    boom_core_top core_u (
        .ap_local_block(ap_local_block),
        .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),
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

    boom_l3_ppa4_memory_system memory_u (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),
        .imem_req_tdata(imem_req_tdata),
        .imem_req_tvalid(imem_req_tvalid),
        .imem_req_tready(imem_req_tready),
        .imem_resp_tdata(imem_resp_tdata),
        .imem_resp_tvalid(imem_resp_tvalid),
        .imem_resp_tready(imem_resp_tready),
        .dmem_req_tdata(dmem_req_tdata),
        .dmem_req_tvalid(dmem_req_tvalid),
        .dmem_req_tready(dmem_req_tready),
        .dmem_resp_tdata(dmem_resp_tdata),
        .dmem_resp_tvalid(dmem_resp_tvalid),
        .dmem_resp_tready(dmem_resp_tready),
        .commit_trace_tdata(commit_trace_tdata),
        .commit_trace_tvalid(commit_trace_tvalid),
        .commit_trace_tready(commit_trace_tready),
        .trace_drain_enable(1'b1),
        .imem_request_count(ppa4_imem_request_count),
        .dmem_read_count(ppa4_dmem_read_count),
        .dmem_write_count(ppa4_dmem_write_count),
        .dmem_response_count(dmem_response_count_unused),
        .commit_trace_count(ppa4_commit_trace_count),
        .commit_trace_checksum(commit_trace_checksum_unused),
        .dmem_fault(ppa4_dmem_fault),
        .imem_fault(ppa4_imem_fault),
        .commit_trace_overflow(ppa4_commit_trace_overflow)
    );
endmodule
