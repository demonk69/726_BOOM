`timescale 1ns/1ps

module exception_recovery_full_core_rtl_tb;
    localparam [63:0] RESET_VECTOR = 64'h10040;
    localparam [63:0] TRAP_VECTOR = 64'h10100;
    reg clk = 0;
    reg rst_n = 0;
    reg [2:0] scenario = 0;
    wire [191:0] imem_req_tdata;
    wire imem_req_tvalid;
    wire imem_req_tready;
    reg [255:0] imem_resp_tdata = 0;
    reg imem_resp_tvalid = 0;
    wire imem_resp_tready;
    wire [319:0] dmem_req_tdata;
    wire dmem_req_tvalid;
    wire [383:0] dmem_resp_tdata = 0;
    wire [767:0] commit_tdata;
    wire commit_tvalid;
    wire io_success, io_halted, io_trap, io_cycle_valid;
    wire [63:0] io_cycle, io_instret;
    reg pending = 0;
    reg [63:0] pending_address = 0;
    reg [31:0] pending_fetch_id = 0;
    reg [31:0] pending_epoch = 0;
    integer cycles;
    integer scenario_arg;
    reg saw_exception, saw_handler, saw_younger;
    reg [63:0] seen_epc, seen_cause;

    assign imem_req_tready = rst_n && !pending && !imem_resp_tvalid;

    boom_core_top dut (
        .ap_local_block(), .ap_local_deadlock(), .ap_clk(clk), .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata), .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(imem_req_tready), .imem_resp_in_TDATA(imem_resp_tdata),
        .imem_resp_in_TVALID(imem_resp_tvalid), .imem_resp_in_TREADY(imem_resp_tready),
        .dmem_req_out_TDATA(dmem_req_tdata), .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(1'b1), .dmem_resp_in_TDATA(dmem_resp_tdata),
        .dmem_resp_in_TVALID(1'b0), .dmem_resp_in_TREADY(),
        .commit_trace_out_TDATA(commit_tdata), .commit_trace_out_TVALID(commit_tvalid),
        .commit_trace_out_TREADY(1'b1), .io_success(io_success), .io_halted(io_halted),
        .io_trap(io_trap), .io_cycle_valid(io_cycle_valid), .io_cycle(io_cycle),
        .io_instret(io_instret)
    );

    always #5 clk = ~clk;

    function automatic [31:0] instruction_at(input [63:0] address);
        begin
            case (address)
                RESET_VECTOR: instruction_at = 32'h00100093;
                RESET_VECTOR + 4: instruction_at = (scenario == 1 || scenario == 5) ?
                    32'h00100073 : (scenario == 2 || scenario == 7) ? 32'h00019002 :
                    (scenario == 3) ? 32'h00000013 : 32'hffffffff;
                RESET_VECTOR + 8: instruction_at = 32'h00200113;
                TRAP_VECTOR: instruction_at = 32'h05a00193;
                TRAP_VECTOR + 4: instruction_at = 32'h00000513;
                TRAP_VECTOR + 8: instruction_at = 32'h00000073;
                default: instruction_at = 32'h00000013;
            endcase
        end
    endfunction

    function automatic [63:0] expected_cause(input [2:0] which);
        begin
            expected_cause = (which == 1 || which == 2 || which == 5 || which == 7) ? 3 :
                             (which == 3) ? 1 : 2;
        end
    endfunction

    always @(posedge clk) begin
        if (!rst_n) begin
            pending <= 0;
            imem_resp_tvalid <= 0;
        end else begin
            if (imem_resp_tvalid && imem_resp_tready) imem_resp_tvalid <= 0;
            if (imem_req_tvalid && imem_req_tready) begin
                $display("PF1_IMEM_REQ scenario=%0d pc=%h fetch_id=%0d epoch=%0d",
                         scenario, imem_req_tdata[63:0], imem_req_tdata[95:64],
                         imem_req_tdata[127:96]);
                pending <= 1;
                pending_address <= imem_req_tdata[63:0];
                pending_fetch_id <= imem_req_tdata[95:64];
                pending_epoch <= imem_req_tdata[127:96];
            end
            if (pending && !imem_resp_tvalid) begin
                imem_resp_tdata <= {
                    (scenario == 3 && pending_address == RESET_VECTOR + 4) ? 64'd1 : 64'd0,
                    31'd0, (scenario == 3 && pending_address == RESET_VECTOR + 4),
                    instruction_at(pending_address), pending_epoch, pending_fetch_id,
                    pending_address};
                imem_resp_tvalid <= 1;
                pending <= 0;
            end
        end
        if (rst_n && commit_tvalid && commit_tdata[0]) begin
            $display("PF1_COMMIT scenario=%0d cycle=%0d pc=%h exception=%b cause=%h rd_valid=%b rd=%0d value=%h",
                     scenario, cycles, commit_tdata[127:64], commit_tdata[256],
                     commit_tdata[383:320], commit_tdata[264],
                     commit_tdata[167:160], commit_tdata[255:192]);
            if (commit_tdata[256]) begin
                saw_exception <= 1;
                seen_epc <= commit_tdata[127:64];
                seen_cause <= commit_tdata[383:320];
            end
            if (!commit_tdata[256] && commit_tdata[127:64] == TRAP_VECTOR &&
                commit_tdata[264] && commit_tdata[167:160] == 3 &&
                commit_tdata[255:192] == 64'h5a) saw_handler <= 1;
            if (!commit_tdata[256] && commit_tdata[127:64] == RESET_VECTOR + 8)
                saw_younger <= 1;
        end
    end

    initial begin
        if (!$value$plusargs("SCENARIO=%d", scenario_arg)) scenario_arg = 0;
        scenario = scenario_arg[2:0];
        saw_exception = 0;
        saw_handler = 0;
        saw_younger = 0;
        seen_epc = 0;
        seen_cause = 0;
        repeat (20) @(posedge clk);
        rst_n = 1;
        cycles = 0;
        while (cycles < 1000000 && io_success !== 1'b1) begin
            @(posedge clk);
            cycles = cycles + 1;
        end
        if (cycles >= 1000000 || io_trap || !saw_exception || !saw_handler || saw_younger ||
            seen_epc != RESET_VECTOR + 4 || seen_cause != expected_cause(scenario)) begin
            $display("PF1_FULL_CORE_RTL_FAIL scenario=%0d cycles=%0d success=%b trap=%b exception=%b handler=%b younger=%b epc=%h cause=%h expected_cause=%h",
                     scenario, cycles, io_success, io_trap, saw_exception, saw_handler,
                     saw_younger, seen_epc, seen_cause, expected_cause(scenario));
        end else begin
            $display("PF1_FULL_CORE_RTL_PASS scenario=%0d cycles=%0d", scenario, cycles);
        end
        $finish;
    end
endmodule
