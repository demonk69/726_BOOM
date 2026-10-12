`timescale 1ns/1ps

module l3r6_capacity_tb;
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;
    localparam [63:0] ENTRY_PC     = 64'h0000_0000_8000_0000;
    localparam [63:0] DATA_BASE    = 64'h0000_0000_8000_0100;
    localparam [63:0] TOHOST_ADDR  = 64'h0000_0000_8000_0080;
    localparam integer IMEM_WORDS = 512;
    localparam integer DMEM_BYTES = 4096;
    localparam integer PENDING_DEPTH = 8;
    localparam integer TARGET_OUTSTANDING = 4;

    reg clk = 1'b0;
    reg rst_n = 1'b0;

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
    wire io_success;
    wire io_halted;
    wire io_trap;
    wire io_cycle_valid;
    wire [63:0] io_cycle;
    wire [63:0] io_instret;

    string program_path;
    string out_dir;
    integer trace_fd;
    integer commit_fd;
    integer memory_fd;
    integer summary_fd;

    integer cycles = 0;
    integer fail_count = 0;
    integer imem_request_count = 0;
    integer imem_response_count = 0;
    integer dmem_request_count = 0;
    integer dmem_load_count = 0;
    integer dmem_store_count = 0;
    integer dmem_response_count = 0;
    integer commit_count = 0;
    integer max_external_outstanding = 0;
    integer max_internal_outstanding = 0;
    integer external_outstanding = 0;
    integer next_request_order = 0;
    integer next_response_order = 0;
    integer capacity_reached_cycle = -1;
    integer first_response_after_capacity_cycle = -1;
    integer post_capacity_load_accepts = 0;
    integer response_reorder_count = 0;
    integer over_capacity_handshakes = 0;
    integer capacity_full_stall_cycles = 0;
    integer protocol_error = 0;
    integer first_fetch_seen = 0;
    integer entry_fetch_seen = 0;
    integer first_fetch_error = 0;
    integer tohost_store_seen = 0;
    integer tohost_commit_seen = 0;
    integer final_a0_valid = 0;
    reg [63:0] final_a0 = 64'd0;

    reg [63:0] program_words [0:IMEM_WORDS-1];
    reg [7:0] dmem [0:DMEM_BYTES-1];

    reg imem_pending;
    reg [63:0] imem_pending_address;
    reg [31:0] imem_pending_fetch_id;
    reg [31:0] imem_pending_epoch;
    reg [1:0] imem_delay;
    reg [255:0] imem_resp_tdata_r;
    reg imem_resp_tvalid_r;

    reg pending_valid [0:PENDING_DEPTH-1];
    reg [31:0] pending_txid [0:PENDING_DEPTH-1];
    reg [63:0] pending_addr [0:PENDING_DEPTH-1];
    reg [63:0] pending_data [0:PENDING_DEPTH-1];
    integer pending_order [0:PENDING_DEPTH-1];
    integer pending_cycle [0:PENDING_DEPTH-1];

    reg [383:0] dmem_resp_tdata_r;
    reg dmem_resp_tvalid_r;
    integer dmem_resp_slot;
    integer release_started = 0;
    integer release_timeout_cycle = 60000;
    integer reorder_done = 0;
    reg held_dmem_req;
    reg [319:0] held_dmem_req_tdata;
    reg held_commit_trace;
    reg [767:0] held_commit_trace_tdata;

    wire dmem_req_is_store = dmem_req_tdata[88] || dmem_req_tdata[47:40] == 8'd1;
    wire dmem_req_is_load = dmem_req_tvalid && !dmem_req_is_store;
    wire at_capacity = external_outstanding >= TARGET_OUTSTANDING;

    assign imem_resp_tdata = imem_resp_tdata_r;
    assign imem_resp_tvalid = imem_resp_tvalid_r;
    assign dmem_resp_tdata = dmem_resp_tdata_r;
    assign dmem_resp_tvalid = dmem_resp_tvalid_r;
    assign commit_trace_tready = rst_n;

    boom_core_top dut (
        .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
        .ap_clk(clk), .ap_rst_n(rst_n),
        .imem_req_out_TDATA(imem_req_tdata), .imem_req_out_TVALID(imem_req_tvalid),
        .imem_req_out_TREADY(imem_req_tready), .imem_resp_in_TDATA(imem_resp_tdata),
        .imem_resp_in_TVALID(imem_resp_tvalid), .imem_resp_in_TREADY(imem_resp_tready),
        .dmem_req_out_TDATA(dmem_req_tdata), .dmem_req_out_TVALID(dmem_req_tvalid),
        .dmem_req_out_TREADY(dmem_req_tready), .dmem_resp_in_TDATA(dmem_resp_tdata),
        .dmem_resp_in_TVALID(dmem_resp_tvalid), .dmem_resp_in_TREADY(dmem_resp_tready),
        .commit_trace_out_TDATA(commit_trace_tdata), .commit_trace_out_TVALID(commit_trace_tvalid),
        .commit_trace_out_TREADY(commit_trace_tready), .io_success(io_success),
        .io_halted(io_halted), .io_trap(io_trap), .io_cycle_valid(io_cycle_valid),
        .io_cycle(io_cycle), .io_instret(io_instret));

    always #5 clk = ~clk;

    function automatic [31:0] instruction_at(input [63:0] address);
        integer word_index;
        integer line_index;
        begin
            case (address)
                RESET_VECTOR: instruction_at = 32'h0010_0293;
                RESET_VECTOR + 64'd4: instruction_at = 32'h01f2_9293;
                RESET_VECTOR + 64'd8: instruction_at = 32'h0002_8067;
                default: begin
                    if (address >= ENTRY_PC && address < ENTRY_PC + (IMEM_WORDS * 8)) begin
                        word_index = (address - ENTRY_PC) >> 2;
                        line_index = word_index >> 1;
                        instruction_at = word_index[0] ? program_words[line_index][63:32] : program_words[line_index][31:0];
                    end else begin
                        instruction_at = 32'h0000_006f;
                    end
                end
            endcase
        end
    endfunction

    function automatic [63:0] read_qword(input [63:0] address);
        integer offset;
        integer byte_index;
        begin
            read_qword = 64'd0;
            offset = address - ENTRY_PC;
            for (byte_index = 0; byte_index < 8; byte_index = byte_index + 1) begin
                if (offset + byte_index >= 0 && offset + byte_index < DMEM_BYTES)
                    read_qword[byte_index * 8 +: 8] = dmem[offset + byte_index];
            end
        end
    endfunction

    task automatic write_qword(input [63:0] address, input [63:0] data, input [7:0] mask);
        integer offset;
        integer byte_index;
        begin
            offset = address - ENTRY_PC;
            for (byte_index = 0; byte_index < 8; byte_index = byte_index + 1) begin
                if (mask[byte_index] && offset + byte_index >= 0 && offset + byte_index < DMEM_BYTES)
                    dmem[offset + byte_index] = data[byte_index * 8 +: 8];
            end
        end
    endtask

    function automatic [383:0] pack_dmem_response(input [31:0] txid, input [63:0] data);
        begin
            pack_dmem_response = 384'd0;
            pack_dmem_response[31:0] = txid;
            pack_dmem_response[127:64] = data;
            pack_dmem_response[191:128] = data;
        end
    endfunction

    function automatic integer pending_count_value;
        integer idx;
        begin
            pending_count_value = 0;
            for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1)
                if (pending_valid[idx]) pending_count_value = pending_count_value + 1;
        end
    endfunction

    function automatic integer have_free_slot;
        integer idx;
        begin
            have_free_slot = 0;
            for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1)
                if (!pending_valid[idx]) have_free_slot = 1;
        end
    endfunction

    assign imem_req_tready = rst_n && !imem_pending && !imem_resp_tvalid_r;
    assign dmem_req_tready = rst_n && have_free_slot() && !(dmem_req_is_load && at_capacity);

    task automatic open_files;
        string path;
        begin
            path = {out_dir, "/full_core_capacity_trace.csv"};
            trace_fd = $fopen(path, "w");
            path = {out_dir, "/capacity_commit_trace.csv"};
            commit_fd = $fopen(path, "w");
            path = {out_dir, "/capacity_final_memory.csv"};
            memory_fd = $fopen(path, "w");
            path = {out_dir, "/capacity_summary.csv"};
            summary_fd = $fopen(path, "w");
            if (trace_fd == 0 || commit_fd == 0 || memory_fd == 0 || summary_fd == 0)
                $fatal(1, "L3R6_CAPACITY_FAIL,cannot_open_reports,out_dir=%s", out_dir);
            $fwrite(trace_fd, "cycle,event,txid,order,address,data,external_outstanding,internal_outstanding,detail\n");
            $fwrite(commit_fd, "cycle,commit_index,pc,instruction,rd,rd_valid,rd_value,memory_valid,is_store,memory_address,memory_data\n");
            $fwrite(memory_fd, "address,observed\n");
            $fwrite(summary_fd, "field,value\n");
        end
    endtask

    task automatic write_final(input string status);
        begin
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd0, read_qword(DATA_BASE + 64'd0));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd8, read_qword(DATA_BASE + 64'd8));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd16, read_qword(DATA_BASE + 64'd16));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd24, read_qword(DATA_BASE + 64'd24));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd32, read_qword(DATA_BASE + 64'd32));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd40, read_qword(DATA_BASE + 64'd40));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", DATA_BASE + 64'd64, read_qword(DATA_BASE + 64'd64));
            $fwrite(memory_fd, "0x%016h,0x%016h\n", TOHOST_ADDR, read_qword(TOHOST_ADDR));
            $fwrite(summary_fd, "status,%s\n", status);
            $fwrite(summary_fd, "cycles,%0d\n", cycles);
            $fwrite(summary_fd, "fail_count,%0d\n", fail_count);
            $fwrite(summary_fd, "dmem_request_count,%0d\n", dmem_request_count);
            $fwrite(summary_fd, "dmem_load_count,%0d\n", dmem_load_count);
            $fwrite(summary_fd, "dmem_store_count,%0d\n", dmem_store_count);
            $fwrite(summary_fd, "dmem_response_count,%0d\n", dmem_response_count);
            $fwrite(summary_fd, "commit_count,%0d\n", commit_count);
            $fwrite(summary_fd, "max_external_outstanding,%0d\n", max_external_outstanding);
            $fwrite(summary_fd, "max_internal_outstanding,%0d\n", max_internal_outstanding);
            $fwrite(summary_fd, "capacity_reached_cycle,%0d\n", capacity_reached_cycle);
            $fwrite(summary_fd, "post_capacity_load_accepts,%0d\n", post_capacity_load_accepts);
            $fwrite(summary_fd, "response_reorder_count,%0d\n", response_reorder_count);
            $fwrite(summary_fd, "over_capacity_handshakes,%0d\n", over_capacity_handshakes);
            $fwrite(summary_fd, "capacity_full_stall_cycles,%0d\n", capacity_full_stall_cycles);
            $fwrite(summary_fd, "tohost_store_seen,%0d\n", tohost_store_seen);
            $fwrite(summary_fd, "tohost_commit_seen,%0d\n", tohost_commit_seen);
            $fwrite(summary_fd, "final_a0_valid,%0d\n", final_a0_valid);
            $fwrite(summary_fd, "final_a0,0x%016h\n", final_a0);
            $fwrite(summary_fd, "protocol_error,%0d\n", protocol_error);
            $fwrite(summary_fd, "first_fetch_seen,%0d\n", first_fetch_seen);
            $fwrite(summary_fd, "entry_fetch_seen,%0d\n", entry_fetch_seen);
            $fwrite(summary_fd, "first_fetch_error,%0d\n", first_fetch_error);
            $fflush(trace_fd);
            $fflush(commit_fd);
            $fflush(memory_fd);
            $fflush(summary_fd);
        end
    endtask

    integer init_i;
    initial begin
        if (!$value$plusargs("PROGRAM=%s", program_path)) program_path = "l3r6_capacity.hex";
        if (!$value$plusargs("OUT_DIR=%s", out_dir)) out_dir = ".";
        for (init_i = 0; init_i < IMEM_WORDS; init_i = init_i + 1) program_words[init_i] = 64'd0;
        for (init_i = 0; init_i < DMEM_BYTES; init_i = init_i + 1) dmem[init_i] = 8'd0;
        for (init_i = 0; init_i < PENDING_DEPTH; init_i = init_i + 1) begin
            pending_valid[init_i] = 1'b0;
            pending_txid[init_i] = 32'd0;
            pending_addr[init_i] = 64'd0;
            pending_data[init_i] = 64'd0;
            pending_order[init_i] = 0;
            pending_cycle[init_i] = 0;
        end
        $readmemh(program_path, program_words);
        write_qword(DATA_BASE + 64'd0, 64'd1, 8'hff);
        write_qword(DATA_BASE + 64'd8, 64'd2, 8'hff);
        write_qword(DATA_BASE + 64'd16, 64'd4, 8'hff);
        write_qword(DATA_BASE + 64'd24, 64'd8, 8'hff);
        write_qword(DATA_BASE + 64'd32, 64'd16, 8'hff);
        write_qword(DATA_BASE + 64'd40, 64'd32, 8'hff);
        open_files();
        repeat (8) @(negedge clk);
        rst_n = 1'b1;
    end

    always @(posedge clk) begin : IMEM_MODEL
        cycles <= cycles + 1;
        if (!rst_n) begin
            imem_pending <= 1'b0;
            imem_resp_tvalid_r <= 1'b0;
            imem_resp_tdata_r <= 256'd0;
            imem_delay <= 2'd0;
        end else begin
            if (imem_req_tvalid && imem_req_tready) begin
                imem_request_count <= imem_request_count + 1;
                imem_pending <= 1'b1;
                imem_pending_address <= imem_req_tdata[63:0];
                imem_pending_fetch_id <= imem_req_tdata[95:64];
                imem_pending_epoch <= imem_req_tdata[127:96];
                imem_delay <= 2'd1;
                $fwrite(trace_fd, "%0d,imem_request,,,%0h,,,%0d,fetch_id=%0d\n", cycles, imem_req_tdata[63:0], dut.state_lsu_outstanding_load_count_V, imem_req_tdata[95:64]);
                if (!first_fetch_seen) begin
                    first_fetch_seen <= 1;
                    if (imem_req_tdata[63:0] != RESET_VECTOR) begin
                        first_fetch_error <= 1;
                        fail_count <= fail_count + 1;
                    end
                end
                if (imem_req_tdata[63:0] == ENTRY_PC) entry_fetch_seen <= 1;
            end
            if (imem_pending && !imem_resp_tvalid_r) begin
                if (imem_delay == 0) begin
                    imem_resp_tdata_r <= {31'd0, 64'd0, 1'b0, instruction_at(imem_pending_address),
                                          imem_pending_epoch, imem_pending_fetch_id, imem_pending_address};
                    imem_resp_tvalid_r <= 1'b1;
                    imem_pending <= 1'b0;
                end else begin
                    imem_delay <= imem_delay - 1'b1;
                end
            end
            if (imem_resp_tvalid_r && imem_resp_tready) begin
                imem_response_count <= imem_response_count + 1;
                $fwrite(trace_fd, "%0d,imem_response,,,%0h,,,%0d,inst=0x%08h\n", cycles, imem_resp_tdata_r[63:0], dut.state_lsu_outstanding_load_count_V, imem_resp_tdata_r[159:128]);
                imem_resp_tvalid_r <= 1'b0;
            end
        end
    end

    always @(posedge clk) begin : DMEM_MODEL
        integer idx;
        integer free_idx;
        integer oldest_idx;
        integer second_idx;
        integer first_order;
        integer second_order;
        integer chosen_idx;
        integer req_is_store;
        integer req_committed;
        reg [31:0] req_txid;
        reg [63:0] req_addr;
        reg [63:0] req_data;
        reg [7:0] req_mask;

        if (!rst_n) begin
            dmem_resp_tvalid_r <= 1'b0;
            dmem_resp_tdata_r <= 384'd0;
            dmem_resp_slot <= 0;
            external_outstanding <= 0;
            release_started <= 0;
            held_dmem_req <= 1'b0;
        end else begin
            if (dut.state_lsu_outstanding_load_count_V > max_internal_outstanding)
                max_internal_outstanding <= dut.state_lsu_outstanding_load_count_V;
            if (external_outstanding > max_external_outstanding)
                max_external_outstanding <= external_outstanding;
            if (external_outstanding >= TARGET_OUTSTANDING && capacity_reached_cycle < 0)
                capacity_reached_cycle <= cycles;
            if (dmem_req_is_load && at_capacity) capacity_full_stall_cycles <= capacity_full_stall_cycles + 1;

            if (held_dmem_req && (!dmem_req_tvalid || dmem_req_tdata !== held_dmem_req_tdata)) begin
                protocol_error <= 1;
                fail_count <= fail_count + 1;
                $fwrite(trace_fd, "%0d,protocol_error,,,,,,%0d,dmem_tdata_changed_while_not_ready\n", cycles, dut.state_lsu_outstanding_load_count_V);
            end
            held_dmem_req <= dmem_req_tvalid && !dmem_req_tready;
            if (dmem_req_tvalid && !dmem_req_tready) held_dmem_req_tdata <= dmem_req_tdata;

            if (dmem_req_tvalid && dmem_req_tready) begin
                req_txid = dmem_req_tdata[31:0];
                req_is_store = dmem_req_tdata[88] || dmem_req_tdata[47:40] == 8'd1;
                req_committed = dmem_req_tdata[104];
                req_addr = dmem_req_tdata[191:128];
                req_data = (dmem_req_tdata[319:256] != 64'd0) ? dmem_req_tdata[319:256] : dmem_req_tdata[255:192];
                req_mask = (dmem_req_tdata[71:64] != 8'd0) ? dmem_req_tdata[71:64] : dmem_req_tdata[63:56];
                dmem_request_count <= dmem_request_count + 1;
                if (req_is_store) begin
                    dmem_store_count <= dmem_store_count + 1;
                    write_qword(req_addr, req_data, req_mask);
                    $fwrite(trace_fd, "%0d,accept_store,0x%08h,,0x%016h,0x%016h,%0d,%0d,committed=%0d mask=0x%02h\n", cycles, req_txid, req_addr, req_data, external_outstanding, dut.state_lsu_outstanding_load_count_V, req_committed, req_mask);
                    if (req_addr == TOHOST_ADDR && req_committed && req_data == 64'd1)
                        tohost_store_seen <= 1;
                end else begin
                    dmem_load_count <= dmem_load_count + 1;
                    if (external_outstanding >= TARGET_OUTSTANDING) begin
                        over_capacity_handshakes <= over_capacity_handshakes + 1;
                        fail_count <= fail_count + 1;
                    end
                    free_idx = -1;
                    for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1)
                        if (!pending_valid[idx] && free_idx < 0) free_idx = idx;
                    if (free_idx < 0) begin
                        fail_count <= fail_count + 1;
                    end else begin
                        pending_valid[free_idx] <= 1'b1;
                        pending_txid[free_idx] <= req_txid;
                        pending_addr[free_idx] <= req_addr;
                        pending_data[free_idx] <= read_qword(req_addr);
                        pending_order[free_idx] <= next_request_order;
                        pending_cycle[free_idx] <= cycles;
                        external_outstanding <= external_outstanding + 1;
                        if (release_started) post_capacity_load_accepts <= post_capacity_load_accepts + 1;
                        $fwrite(trace_fd, "%0d,accept_load,0x%08h,%0d,0x%016h,0x%016h,%0d,%0d,\n", cycles, req_txid, next_request_order, req_addr, read_qword(req_addr), external_outstanding + 1, dut.state_lsu_outstanding_load_count_V);
                        next_request_order <= next_request_order + 1;
                    end
                end
            end

            if (!release_started && (external_outstanding >= TARGET_OUTSTANDING || cycles > release_timeout_cycle)) begin
                release_started <= 1;
            end

            if (!dmem_resp_tvalid_r && release_started) begin
                oldest_idx = -1;
                second_idx = -1;
                first_order = 32'h7fff_ffff;
                second_order = 32'h7fff_ffff;
                for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1) begin
                    if (pending_valid[idx]) begin
                        if (pending_order[idx] < first_order) begin
                            second_order = first_order;
                            second_idx = oldest_idx;
                            first_order = pending_order[idx];
                            oldest_idx = idx;
                        end else if (pending_order[idx] < second_order) begin
                            second_order = pending_order[idx];
                            second_idx = idx;
                        end
                    end
                end
                chosen_idx = -1;
                if (!reorder_done && pending_count_value() >= 2 && second_idx >= 0)
                    chosen_idx = second_idx;
                else if (oldest_idx >= 0)
                    chosen_idx = oldest_idx;
                if (chosen_idx >= 0) begin
                    dmem_resp_tdata_r <= pack_dmem_response(pending_txid[chosen_idx], pending_data[chosen_idx]);
                    dmem_resp_tvalid_r <= 1'b1;
                    dmem_resp_slot <= chosen_idx;
                end
            end

            if (dmem_resp_tvalid_r && dmem_resp_tready) begin
                dmem_response_count <= dmem_response_count + 1;
                if (first_response_after_capacity_cycle < 0 && capacity_reached_cycle >= 0)
                    first_response_after_capacity_cycle <= cycles;
                if (pending_order[dmem_resp_slot] != next_response_order) begin
                    response_reorder_count <= response_reorder_count + 1;
                    reorder_done <= 1;
                end
                $fwrite(trace_fd, "%0d,response_load,0x%08h,%0d,0x%016h,0x%016h,%0d,%0d,response_order=%0d status=%s\n", cycles, pending_txid[dmem_resp_slot], pending_order[dmem_resp_slot], pending_addr[dmem_resp_slot], pending_data[dmem_resp_slot], external_outstanding - 1, dut.state_lsu_outstanding_load_count_V, next_response_order, pending_order[dmem_resp_slot] != next_response_order ? "REORDERED" : "IN_ORDER");
                pending_valid[dmem_resp_slot] <= 1'b0;
                external_outstanding <= external_outstanding - 1;
                next_response_order <= next_response_order + 1;
                dmem_resp_tvalid_r <= 1'b0;
            end
        end
    end

    always @(posedge clk) begin : COMMIT_MONITOR
        if (!rst_n) begin
            held_commit_trace <= 1'b0;
        end else begin
            if (held_commit_trace && (!commit_trace_tvalid || commit_trace_tdata !== held_commit_trace_tdata)) begin
                protocol_error <= 1;
                fail_count <= fail_count + 1;
            end
            held_commit_trace <= commit_trace_tvalid && !commit_trace_tready;
            if (commit_trace_tvalid && !commit_trace_tready) held_commit_trace_tdata <= commit_trace_tdata;
            if (commit_trace_tvalid && commit_trace_tready && commit_trace_tdata[0]) begin
                $fwrite(commit_fd, "%0d,%0d,0x%016h,0x%08h,%0d,%0d,0x%016h,%0d,%0d,0x%016h,0x%016h\n",
                    cycles, commit_count, commit_trace_tdata[127:64], commit_trace_tdata[159:128],
                    commit_trace_tdata[167:160], commit_trace_tdata[264], commit_trace_tdata[255:192],
                    commit_trace_tdata[392], commit_trace_tdata[400], commit_trace_tdata[511:448], commit_trace_tdata[575:512]);
                $fwrite(trace_fd, "%0d,commit,,%0d,0x%016h,0x%016h,%0d,%0d,inst=0x%08h rd=%0d rd_valid=%0d\n", cycles, commit_count, commit_trace_tdata[127:64], commit_trace_tdata[255:192], external_outstanding, dut.state_lsu_outstanding_load_count_V, commit_trace_tdata[159:128], commit_trace_tdata[167:160], commit_trace_tdata[264]);
                if (commit_trace_tdata[264] && commit_trace_tdata[167:160] == 8'd10) begin
                    final_a0 <= commit_trace_tdata[255:192];
                    final_a0_valid <= 1;
                end
                if (commit_trace_tdata[392] && commit_trace_tdata[400] && commit_trace_tdata[511:448] == TOHOST_ADDR)
                    tohost_commit_seen <= 1;
                commit_count <= commit_count + 1;
            end
        end
    end

    always @(posedge clk) begin : FINISH_CONTROL
        if (cycles > 1000000) begin
            fail_count <= fail_count + 1;
            write_final("TIMEOUT");
            $display("L3R6_CAPACITY,FAIL,timeout,max_external=%0d,max_internal=%0d", max_external_outstanding, max_internal_outstanding);
            $finish;
        end
        if (rst_n && io_trap) begin
            fail_count <= fail_count + 1;
            write_final("TRAP");
            $display("L3R6_CAPACITY,FAIL,trap,cycle=%0d", cycles);
            $finish;
        end
        if (rst_n && tohost_store_seen && tohost_commit_seen) begin
            repeat (20) @(posedge clk);
            write_final("COMPLETE");
            $display("L3R6_CAPACITY,COMPLETE,cycles=%0d,max_external=%0d,max_internal=%0d,reorders=%0d,post_capacity_loads=%0d", cycles, max_external_outstanding, max_internal_outstanding, response_reorder_count, post_capacity_load_accepts);
            $finish;
        end
    end
endmodule
