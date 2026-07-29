`timescale 1ns/1ps

module boom_core_rtl_tb;
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;

    reg clk;
    reg rst_n;
    reg [7:0] scenario_code;
    integer max_cycles;
    integer cycles;
    integer reset_count;
    integer reset_release_count;
    reg first_fetch_seen;
    reg first_fetch_after_mid_reset_seen;
    reg first_fetch_error;
    reg mid_reset_done;
    reg trigger_required;
    string scenario;
    string program_name;

    wire tohost_seen;
    wire [63:0] tohost_value;
    wire protocol_error;
    wire imem_pending;
    wire dmem_pending;
    wire dmem_stalled;
    wire trace_stalled;
    wire branch_commit_visible;
    wire branch_recovery_visible;
    wire rob_nonempty;
    wire tohost_commit_seen;
    wire [31:0] imem_transfers;
    wire [31:0] dmem_transfers;
    wire [31:0] dmem_loads;
    wire [31:0] dmem_stores;
    wire [31:0] commit_count;
    wire [127:0] observed_imem_req;
    wire observed_imem_transfer;

    boom_core_rtl_harness harness (
        .clk(clk), .rst_n(rst_n), .scenario_code(scenario_code),
        .tohost_seen(tohost_seen), .tohost_value(tohost_value), .protocol_error(protocol_error),
        .imem_pending(imem_pending), .dmem_pending(dmem_pending),
        .dmem_stalled(dmem_stalled), .trace_stalled(trace_stalled),
        .branch_commit_visible(branch_commit_visible),
        .branch_recovery_visible(branch_recovery_visible), .rob_nonempty(rob_nonempty),
        .tohost_commit_seen(tohost_commit_seen),
        .imem_transfers(imem_transfers),
        .dmem_transfers(dmem_transfers), .dmem_loads(dmem_loads), .dmem_stores(dmem_stores),
        .commit_count(commit_count), .observed_imem_req(observed_imem_req),
        .observed_imem_transfer(observed_imem_transfer)
    );

    function automatic [7:0] scenario_number(input string name);
        begin
            if (name == "R1_RESET_FRONTEND_OUTSTANDING") scenario_number = 8'd1;
            else if (name == "R2_RESET_ROB_NONEMPTY") scenario_number = 8'd2;
            else if (name == "R3_RESET_IQ_NONEMPTY") scenario_number = 8'd3;
            else if (name == "R4_RESET_LOAD_PENDING") scenario_number = 8'd4;
            else if (name == "R5_RESET_STORE_PENDING") scenario_number = 8'd5;
            else if (name == "R6_RESET_BRANCH_RECOVERY") scenario_number = 8'd6;
            else if (name == "R7_RESET_TRACE_BACKPRESSURE") scenario_number = 8'd7;
            else if (name == "B0_TRACE_READY_ALWAYS") scenario_number = 8'd10;
            else if (name == "B1_TRACE_STALL_1") scenario_number = 8'd11;
            else if (name == "B2_TRACE_STALL_BURST") scenario_number = 8'd12;
            else if (name == "B3_TRACE_RANDOM_STALL") scenario_number = 8'd13;
            else if (name == "B4_TRACE_STALL_AT_BRANCH_COMMIT") scenario_number = 8'd14;
            else if (name == "B5_TRACE_STALL_AT_STORE_COMMIT") scenario_number = 8'd15;
            else if (name == "I0_IMEM_READY_ALWAYS") scenario_number = 8'd20;
            else if (name == "I1_IMEM_REQ_STALL") scenario_number = 8'd21;
            else if (name == "I2_IMEM_RESPONSE_DELAY_1") scenario_number = 8'd22;
            else if (name == "I3_IMEM_RESPONSE_DELAY_4") scenario_number = 8'd23;
            else if (name == "I4_IMEM_RANDOM_DELAY") scenario_number = 8'd24;
            else if (name == "I5_IMEM_STALE_RESPONSE_AFTER_REDIRECT") scenario_number = 8'd25;
            else if (name == "I6_IMEM_RESPONSE_DURING_RESET") scenario_number = 8'd26;
            else if (name == "D0_DMEM_READY_ALWAYS") scenario_number = 8'd30;
            else if (name == "D1_STORE_REQ_STALL") scenario_number = 8'd31;
            else if (name == "D2_LOAD_REQ_STALL") scenario_number = 8'd32;
            else if (name == "D3_LOAD_RESPONSE_DELAY") scenario_number = 8'd33;
            else if (name == "D4_LOAD_RANDOM_DELAY") scenario_number = 8'd34;
            else if (name == "D5_STALE_LOAD_RESPONSE_AFTER_FLUSH") scenario_number = 8'd35;
            else if (name == "D6_STORE_DURING_BRANCH_FLUSH") scenario_number = 8'd36;
            else if (name == "D7_DMEM_RESPONSE_DURING_RESET") scenario_number = 8'd37;
            else if (name == "D8_TOHOST_STORE_BACKPRESSURE") scenario_number = 8'd38;
            else if (name == "P0_RESET_AND_BRANCH_MISPREDICT") scenario_number = 8'd40;
            else if (name == "P1_RESET_AND_LOAD_RESPONSE") scenario_number = 8'd41;
            else if (name == "P2_RESET_AND_STORE_ACCEPT") scenario_number = 8'd42;
            else if (name == "P3_EXCEPTION_AND_BRANCH_MISPREDICT") scenario_number = 8'd43;
            else if (name == "P4_BRANCH_MISPREDICT_AND_LOAD_RESPONSE") scenario_number = 8'd44;
            else if (name == "P5_COMMIT_AND_TRACE_BACKPRESSURE") scenario_number = 8'd45;
            else if (name == "P6_COMMIT_AND_STORE_BACKPRESSURE") scenario_number = 8'd46;
            else if (name == "P7_REDIRECT_AND_IMEM_RESPONSE") scenario_number = 8'd47;
            else scenario_number = 8'd0;
        end
    endfunction

    function automatic is_mid_reset(input [7:0] code);
        begin
            is_mid_reset = (code >= 8'd1 && code <= 8'd7) ||
                           code == 8'd26 || code == 8'd37 ||
                           (code >= 8'd40 && code <= 8'd42);
        end
    endfunction

    task automatic pulse_mid_reset;
        begin
            @(negedge clk);
            rst_n = 1'b0;
            reset_count = reset_count + 1;
            repeat (4) @(negedge clk);
            rst_n = 1'b1;
            reset_release_count = reset_release_count + 1;
            mid_reset_done = 1'b1;
        end
    endtask

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    always @(posedge clk) begin
        if (rst_n && observed_imem_transfer) begin
            if (!first_fetch_seen) begin
                first_fetch_seen <= 1'b1;
                if (observed_imem_req[63:0] != RESET_VECTOR) begin
                    first_fetch_error <= 1'b1;
                    $error("power-on first fetch was %h, expected %h", observed_imem_req[63:0], RESET_VECTOR);
                end
            end
            if (reset_release_count > 1 && !first_fetch_after_mid_reset_seen) begin
                first_fetch_after_mid_reset_seen <= 1'b1;
                if (observed_imem_req[63:0] != RESET_VECTOR) begin
                    first_fetch_error <= 1'b1;
                    $error("mid-run reset first fetch was %h, expected %h", observed_imem_req[63:0], RESET_VECTOR);
                end
            end
        end
    end

    initial begin
        if (!$value$plusargs("SCENARIO=%s", scenario)) scenario = "R0_POWER_ON_RESET";
        if (!$value$plusargs("PROGRAM_NAME=%s", program_name)) program_name = "independent_alu";
        if (!$value$plusargs("MAX_CYCLES=%d", max_cycles)) max_cycles = 12000;
        scenario_code = scenario_number(scenario);
        rst_n = 1'b0;
        cycles = 0;
        reset_count = 1;
        reset_release_count = 0;
        first_fetch_seen = 1'b0;
        first_fetch_after_mid_reset_seen = 1'b0;
        first_fetch_error = 1'b0;
        mid_reset_done = 1'b0;
        trigger_required = is_mid_reset(scenario_code);

        $display("GATE3_8_START scenario=%s code=%0d program=%s max_cycles=%0d",
                 scenario, scenario_code, program_name, max_cycles);
        repeat (5) @(negedge clk);
        rst_n = 1'b1;
        reset_release_count = 1;

        if (trigger_required) begin
            fork
                begin
                    wait ((scenario_code == 8'd1 && imem_pending) ||
                          (scenario_code == 8'd2 && rob_nonempty) ||
                          (scenario_code == 8'd3 && imem_transfers >= 3 && commit_count < 3) ||
                          (scenario_code == 8'd4 && dmem_pending) ||
                          (scenario_code == 8'd5 && dmem_stalled) ||
                          (scenario_code == 8'd6 && branch_recovery_visible) ||
                          (scenario_code == 8'd7 && trace_stalled) ||
                          (scenario_code == 8'd26 && imem_pending) ||
                          (scenario_code == 8'd37 && dmem_pending) ||
                          (scenario_code == 8'd40 && branch_recovery_visible) ||
                          (scenario_code == 8'd41 && dmem_pending) ||
                          (scenario_code == 8'd42 && dmem_stalled));
                    pulse_mid_reset();
                end
            join_none
        end

        while (cycles < max_cycles && !(tohost_seen && tohost_value != 0 && tohost_commit_seen)) begin
            @(posedge clk);
            cycles = cycles + 1;
        end

        if (cycles >= max_cycles) begin
            harness.trace_monitor.finish_trace("timeout");
            $fatal(1,
                "GATE3_8_TIMEOUT scenario=%s cycles=%0d commits=%0d imem=%0d dmem=%0d resets=%0d",
                scenario, cycles, commit_count, imem_transfers, dmem_transfers, reset_count);
        end
        if (trigger_required && !mid_reset_done) begin
            harness.trace_monitor.finish_trace("reset_trigger_not_reached");
            $fatal(1, "GATE3_8_RESET_TRIGGER_NOT_REACHED scenario=%s", scenario);
        end
        if (tohost_value != 64'd1 || protocol_error || first_fetch_error) begin
            harness.trace_monitor.finish_trace("failed");
            $fatal(1,
                "GATE3_8_FAIL scenario=%s tohost=%h protocol_error=%0d first_fetch_error=%0d",
                scenario, tohost_value, protocol_error, first_fetch_error);
        end
        if (trigger_required && !first_fetch_after_mid_reset_seen) begin
            harness.trace_monitor.finish_trace("no_post_reset_fetch");
            $fatal(1, "GATE3_8_NO_POST_RESET_FETCH scenario=%s", scenario);
        end

        harness.trace_monitor.finish_trace("pass");
        $display(
            "GATE3_8_PASS scenario=%s cycles=%0d commits=%0d resets=%0d imem=%0d dmem=%0d loads=%0d stores=%0d tohost=%h",
            scenario, cycles, commit_count, reset_count, imem_transfers, dmem_transfers,
            dmem_loads, dmem_stores, tohost_value);
        $finish;
    end
endmodule
