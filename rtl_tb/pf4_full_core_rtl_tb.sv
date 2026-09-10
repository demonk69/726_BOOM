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
    integer cycles, max_cycles, init_file, scan_status, init_count, i, repeats;
    integer branch_updates, mispredicts, exceptions;
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
        .fault_sent(fault_sent), .trap_vector_requested(trap_vector_requested));

    initial begin clk = 0; forever #5 clk = ~clk; end
    always @(posedge clk) if (rst_n) begin
        if (brupdate_valid) begin
            branch_updates = branch_updates + 1;
            if (brupdate_mispredict) mispredicts = mispredicts + 1;
        end
        if (exception_valid) exceptions = exceptions + 1;
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
        repeat (5) @(negedge clk); rst_n = 1;
        wait (reset_completed);
        for (i = 0; i < init_count; i = i + 1) begin
            seed_pc = RESET_VECTOR + init_offset[i];
            repeats = init_counter[i] == 3 ? 2 : (init_counter[i] == 2 ? 1 : 0);
            repeat (repeats) send_seed(seed_pc, 1'b1);
        end
        @(negedge clk); start_fetch = 1;
        while (cycles < max_cycles &&
               !(fault_mode == 2 && program_name == "pf4_pred_t_actual_nt_fault" ?
                 (fault_sent && exceptions != 0) : (tohost_seen && tohost_commit_seen))) begin
            @(posedge clk); cycles = cycles + 1;
        end
        @(posedge clk);
        if (cycles >= max_cycles || protocol_error)
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL program=%s timeout/protocol", program_name);
        if (!(fault_mode == 2 && program_name == "pf4_pred_t_actual_nt_fault") &&
            (tohost_value != 1 || !tohost_commit_seen))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL program=%s completion", program_name);
        if (program_name == "pf4_pred_t_actual_t" &&
            (fault_site_requested || fault_sent || exceptions != 0))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL predicted-T actual-T exposed younger fault");
        if (program_name == "pf4_pred_t_actual_nt_fault" &&
            (!fault_site_requested || !fault_sent || exceptions == 0 || mispredicts == 0))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL predicted-T actual-NT did not refetch fault");
        if (program_name == "pf4_fault_refetch" &&
            (!fault_sent || exceptions == 0 || !trap_vector_requested))
            $fatal(1, "PF4_FULL_CORE_RTL_FAIL fault refetch contract");
        for (i = 0; i < init_count; i = i + 1) begin
            if (init_counter[i] >= 2 &&
                (harness.dut.state_predictor_valid_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] !== 1'b1 ||
                 harness.dut.state_predictor_counters_s_U.ram[((RESET_VECTOR + init_offset[i]) >> 1) & 8'hff] !== init_counter[i]))
                $fatal(1, "PF4_FULL_CORE_RTL_FAIL BIM changed program=%s entry=%0d", program_name, i);
        end
        harness.trace_monitor.finish_trace("pass");
        $display("PF4_FULL_CORE_RTL_PASS program=%s branches=%0d mispredicts=%0d exceptions=%0d commits=%0d cycles=%0d bim_entries=%0d",
                 program_name, branch_updates, mispredicts, exceptions, commit_count, cycles, init_count);
        $finish;
    end
endmodule
