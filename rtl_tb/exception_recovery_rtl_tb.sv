`timescale 1ns/1ps

module exception_recovery_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg [63:0] fault_pc = 0;
    reg [63:0] fault_cause = 0;
    reg [31:0] fault_inst = 0;
    reg [7:0] rob_head = 0;
    wire ap_done, ap_idle, ap_ready, ap_local_block, ap_local_deadlock;
    wire [63:0] exception_pc, exception_cause, exception_target, observable;
    wire exception_pc_ap_vld, exception_cause_ap_vld;
    wire exception_target_ap_vld, observable_ap_vld;
    integer i;
    integer failures = 0;

    synth_exception_recovery_top dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(ap_clk), .ap_rst(ap_rst),
        .ap_start(ap_start), .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .fault_pc(fault_pc), .fault_cause(fault_cause), .fault_inst(fault_inst),
        .rob_head(rob_head), .exception_pc(exception_pc),
        .exception_pc_ap_vld(exception_pc_ap_vld),
        .exception_cause(exception_cause),
        .exception_cause_ap_vld(exception_cause_ap_vld),
        .exception_target(exception_target),
        .exception_target_ap_vld(exception_target_ap_vld),
        .observable(observable), .observable_ap_vld(observable_ap_vld)
    );

    always #5 ap_clk = ~ap_clk;

    task invoke;
        integer guard;
        begin
            @(negedge ap_clk); ap_start = 1;
            @(negedge ap_clk); ap_start = 0;
            guard = 0;
            while (ap_done !== 1'b1 && guard < 256) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1) begin
                failures = failures + 1;
                $display("CASE_FAIL,pf1_timeout_%0d", i);
            end
        end
    endtask

    initial begin
        repeat (4) @(negedge ap_clk);
        ap_rst = 0;
        for (i = 0; i < 64; i = i + 1) begin
            fault_pc = 64'h1000 + i * 2;
            fault_cause = i % 4;
            fault_inst = (fault_cause == 3) ? 32'h00100073 : 32'hffffffff;
            rob_head = (29 + i) % 32;
            invoke();
            if (!ap_done || !ap_ready || !exception_pc_ap_vld ||
                !exception_cause_ap_vld || !exception_target_ap_vld ||
                !observable_ap_vld || exception_pc !== fault_pc ||
                exception_cause !== fault_cause || exception_target !== 64'h10100 ||
                observable[8:0] !== 9'h1ff) begin
                failures = failures + 1;
                $display("CASE_FAIL,pf1_%0d", i);
            end else begin
                $display("CASE_PASS,pf1_%0d,precise_take_epc_cause_recovery", i);
            end
        end
        if (failures == 0) $display("PF1_EXCEPTION_RTL_PASS cases=64");
        else $display("PF1_EXCEPTION_RTL_FAIL failures=%0d", failures);
        $finish;
    end
endmodule
