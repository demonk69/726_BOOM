`timescale 1ns/1ps

module commit_trace_monitor (
    input  wire         clk,
    input  wire         rst_n,
    input  wire [7:0]   scenario_code,
    input  wire [127:0] imem_req_tdata,
    input  wire         imem_req_tvalid,
    input  wire         imem_req_tready,
    input  wire [319:0] dmem_req_tdata,
    input  wire         dmem_req_tvalid,
    input  wire         dmem_req_tready,
    input  wire [767:0] trace_tdata,
    input  wire         trace_tvalid,
    output wire         trace_tready,
    output reg  [31:0]  commit_count,
    output reg  [31:0]  trace_transfer_count,
    output reg          protocol_error,
    output reg          tohost_commit_seen,
    output wire         branch_commit_visible
);
    integer trace_fd;
    integer cycle_count;
    integer stall_seen;
    reg [31:0] lfsr;
    reg held_trace;
    reg [767:0] held_trace_data;
    reg reset_was_active;
    string trace_file;
    string scenario_name;
    string program_name;

    wire [31:0] commit_inst = trace_tdata[159:128];
    wire [6:0] commit_opcode = commit_inst[6:0];
    wire commit_is_branch = commit_opcode == 7'h63 || commit_opcode == 7'h6f || commit_opcode == 7'h67;
    wire commit_is_store = trace_tdata[392] && trace_tdata[400];
    wire targeted_commit = (scenario_code == 8'd14 && commit_is_branch) ||
                           (scenario_code == 8'd15 && commit_is_store);
    wire burst_stall = scenario_code == 8'd12 && cycle_count >= 5000 && cycle_count < 9000;
    wire random_stall = scenario_code == 8'd13 && !lfsr[0];
    wire reset_priority_stall = (scenario_code == 8'd7 || scenario_code == 8'd45) && stall_seen < 3;
    wire one_cycle_stall = scenario_code == 8'd11 && stall_seen == 0;

    assign trace_tready = rst_n && !(trace_tvalid &&
        (burst_stall || random_stall || (targeted_commit && stall_seen < 3) ||
         reset_priority_stall || one_cycle_stall));
    assign branch_commit_visible = trace_tvalid && commit_is_branch && trace_tdata[384];

    function automatic string json_bool(input value);
        begin
            json_bool = (value === 1'b1) ? "true" : "false";
        end
    endfunction

    initial begin
        if (!$value$plusargs("TRACE=%s", trace_file)) trace_file = "gate3_8_rtl_trace.jsonl";
        if (!$value$plusargs("SCENARIO=%s", scenario_name)) scenario_name = "R0_POWER_ON_RESET";
        if (!$value$plusargs("PROGRAM_NAME=%s", program_name)) program_name = "independent_alu";
        trace_fd = $fopen(trace_file, "w");
        if (trace_fd == 0) $fatal(1, "cannot open RTL trace %s", trace_file);
        $fwrite(trace_fd,
            "{\"cycle\":0,\"event\":\"metadata\",\"phase\":\"start\",\"source\":\"rtl_xsim\",\"program\":\"%s\",\"scenario\":\"%s\",\"status\":\"running\"}\n",
            program_name, scenario_name);
        cycle_count = 0;
        stall_seen = 0;
        lfsr = 32'h1ace_b00c;
        held_trace = 1'b0;
        held_trace_data = 0;
        reset_was_active = 1'b1;
        commit_count = 0;
        trace_transfer_count = 0;
        protocol_error = 1'b0;
        tohost_commit_seen = 1'b0;
    end

    always @(posedge clk) begin
        cycle_count <= cycle_count + 1;
        lfsr <= {lfsr[30:0], lfsr[31] ^ lfsr[21] ^ lfsr[1] ^ lfsr[0]};

        if (!rst_n && !reset_was_active) begin
            $fwrite(trace_fd,
                "{\"cycle\":%0d,\"event\":\"reset\",\"reset\":true,\"scenario\":\"%s\"}\n",
                cycle_count, scenario_name);
        end else if (rst_n && reset_was_active) begin
            $fwrite(trace_fd,
                "{\"cycle\":%0d,\"event\":\"reset\",\"reset\":false,\"scenario\":\"%s\"}\n",
                cycle_count, scenario_name);
        end
        reset_was_active <= !rst_n;

        if (trace_tvalid && !trace_tready && rst_n) stall_seen <= stall_seen + 1;

        if (held_trace && (!trace_tvalid || trace_tdata !== held_trace_data)) begin
            protocol_error <= 1'b1;
            $error("commit trace TVALID/TDATA changed while TREADY was low");
        end
        held_trace <= trace_tvalid && !trace_tready && rst_n;
        if (trace_tvalid && !trace_tready && rst_n) held_trace_data <= trace_tdata;
        if (!rst_n) held_trace <= 1'b0;

        if (imem_req_tvalid && imem_req_tready) begin
            $fwrite(trace_fd,
                "{\"cycle\":%0d,\"event\":\"imem_request\",\"pc\":\"0x%016h\",\"instruction\":null,\"rob_idx\":null,\"rd_valid\":false,\"rd\":null,\"rd_value\":null,\"exception\":false,\"exception_cause\":null,\"branch_taken\":null,\"branch_target\":null,\"branch_mispredict\":null,\"redirect\":false,\"flush\":false,\"memory_valid\":false,\"memory_address\":null,\"memory_data\":null,\"memory_mask\":null,\"imem_req_valid\":true,\"imem_req_ready\":true,\"dmem_req_valid\":%s,\"dmem_req_ready\":%s,\"commit_trace_valid\":%s,\"commit_trace_ready\":%s,\"reset\":false,\"fetch_id\":%0d}\n",
                cycle_count, imem_req_tdata[63:0], json_bool(dmem_req_tvalid), json_bool(dmem_req_tready),
                json_bool(trace_tvalid), json_bool(trace_tready), imem_req_tdata[95:64]);
        end

        if (dmem_req_tvalid && dmem_req_tready) begin
            $fwrite(trace_fd,
                "{\"cycle\":%0d,\"event\":\"dmem_request\",\"pc\":null,\"instruction\":null,\"rob_idx\":%0d,\"rd_valid\":false,\"rd\":null,\"rd_value\":null,\"exception\":false,\"exception_cause\":null,\"branch_taken\":null,\"branch_target\":null,\"branch_mispredict\":null,\"redirect\":false,\"flush\":false,\"memory_valid\":true,\"memory_address\":\"0x%016h\",\"memory_data\":\"0x%016h\",\"memory_mask\":\"0x%02h\",\"imem_req_valid\":%s,\"imem_req_ready\":%s,\"dmem_req_valid\":true,\"dmem_req_ready\":true,\"commit_trace_valid\":%s,\"commit_trace_ready\":%s,\"reset\":false,\"transaction_id\":%0d,\"is_store\":%s,\"committed\":%s}\n",
                cycle_count, dmem_req_tdata[39:32], dmem_req_tdata[191:128],
                dmem_req_tdata[319:256] != 0 ? dmem_req_tdata[319:256] : dmem_req_tdata[255:192],
                dmem_req_tdata[71:64] != 0 ? dmem_req_tdata[71:64] : dmem_req_tdata[63:56],
                json_bool(imem_req_tvalid), json_bool(imem_req_tready), json_bool(trace_tvalid),
                json_bool(trace_tready), dmem_req_tdata[31:0], json_bool(dmem_req_tdata[88]),
                json_bool(dmem_req_tdata[104]));
            if (dmem_req_tdata[88] && dmem_req_tdata[104] && dmem_req_tdata[191:128] == 64'h8000_0080) begin
                $fwrite(trace_fd,
                    "{\"cycle\":%0d,\"event\":\"tohost\",\"source\":\"rtl_xsim\",\"program\":\"%s\",\"address\":\"0x%016h\",\"value\":\"0x%016h\",\"mask\":\"0x%02h\",\"command\":\"store\",\"committed\":true}\n",
                    cycle_count, program_name, dmem_req_tdata[191:128], dmem_req_tdata[319:256],
                    dmem_req_tdata[71:64]);
            end
        end

        if (trace_tvalid && trace_tready) begin
            trace_transfer_count <= trace_transfer_count + 1;
            if (trace_tdata[0]) begin
                commit_count <= commit_count + 1;
                if (trace_tdata[392] && trace_tdata[400] && trace_tdata[511:448] == 64'h8000_0080)
                    tohost_commit_seen <= 1'b1;
                $fwrite(trace_fd,
                    "{\"cycle\":%0d,\"event\":\"commit\",\"source\":\"rtl_xsim\",\"program\":\"%s\",\"scenario\":\"%s\",\"pc\":\"0x%016h\",\"instruction\":\"0x%08h\",\"rob_idx\":null,\"rd_valid\":%s,\"rd\":%0d,\"rd_value\":\"0x%016h\",\"exception\":%s,\"exception_cause\":\"0x%016h\",\"branch_taken\":null,\"branch_target\":null,\"branch_mispredict\":%s,\"redirect\":false,\"flush\":false,\"memory_valid\":%s,\"memory_address\":\"0x%016h\",\"memory_data\":\"0x%016h\",\"memory_mask\":\"0x%02h\",\"is_store\":%s,\"imem_req_valid\":%s,\"imem_req_ready\":%s,\"dmem_req_valid\":%s,\"dmem_req_ready\":%s,\"commit_trace_valid\":true,\"commit_trace_ready\":true,\"reset\":false}\n",
                    cycle_count, program_name, scenario_name, trace_tdata[127:64], trace_tdata[159:128],
                    json_bool(trace_tdata[264]), trace_tdata[167:160], trace_tdata[255:192],
                    json_bool(trace_tdata[256]), trace_tdata[383:320], json_bool(trace_tdata[384]),
                    json_bool(trace_tdata[392]), trace_tdata[511:448], trace_tdata[575:512],
                    trace_tdata[711:704], json_bool(trace_tdata[400]), json_bool(imem_req_tvalid),
                    json_bool(imem_req_tready), json_bool(dmem_req_tvalid), json_bool(dmem_req_tready));
            end
        end
    end

    task finish_trace(input string status);
        begin
            $fwrite(trace_fd,
                "{\"cycle\":%0d,\"event\":\"metadata\",\"phase\":\"end\",\"source\":\"rtl_xsim\",\"program\":\"%s\",\"scenario\":\"%s\",\"status\":\"%s\",\"commit_count\":%0d,\"trace_transfers\":%0d}\n",
                cycle_count, program_name, scenario_name, status, commit_count, trace_transfer_count);
            $fclose(trace_fd);
        end
    endtask
endmodule
