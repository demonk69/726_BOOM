`timescale 1 ns / 1 ps

module boom_l3_ppa4_memory_system #(
    parameter integer IMEM_WORDS = 1024,
    parameter integer DMEM_BYTES = 8192,
    parameter integer PENDING_DEPTH = 8,
    parameter integer STORE_DEPTH = 8,
    parameter integer TRACE_DEPTH = 16
) (
    input wire ap_clk,
    input wire ap_rst_n,

    input wire [191:0] imem_req_tdata,
    input wire imem_req_tvalid,
    output wire imem_req_tready,
    output reg [255:0] imem_resp_tdata,
    output reg imem_resp_tvalid,
    input wire imem_resp_tready,

    input wire [319:0] dmem_req_tdata,
    input wire dmem_req_tvalid,
    output wire dmem_req_tready,
    output reg [383:0] dmem_resp_tdata,
    output reg dmem_resp_tvalid,
    input wire dmem_resp_tready,

    input wire [767:0] commit_trace_tdata,
    input wire commit_trace_tvalid,
    output wire commit_trace_tready,
    input wire trace_drain_enable,

    output reg [31:0] imem_request_count,
    output reg [31:0] dmem_read_count,
    output reg [31:0] dmem_write_count,
    output reg [31:0] dmem_response_count,
    output reg [31:0] commit_trace_count,
    output reg [31:0] commit_trace_checksum,
    output reg dmem_fault,
    output reg imem_fault,
    output reg commit_trace_overflow
);

    localparam integer TXID_LSB = 0;
    localparam integer COMMAND_LSB = 40;
    localparam integer MASK_LSB = 56;
    localparam integer WRITE_MASK_LSB = 64;
    localparam integer IS_STORE_BIT = 88;
    localparam integer COMMITTED_BIT = 104;
    localparam integer ADDRESS_LSB = 128;
    localparam integer DATA_LSB = 192;
    localparam integer WRITE_DATA_LSB = 256;

    reg pending_valid [0:PENDING_DEPTH-1];
    reg [31:0] pending_txid [0:PENDING_DEPTH-1];
    reg [63:0] pending_data [0:PENDING_DEPTH-1];
    reg pending_exception [0:PENDING_DEPTH-1];
    reg [63:0] pending_cause [0:PENDING_DEPTH-1];
    reg [3:0] pending_latency [0:PENDING_DEPTH-1];

    reg store_valid [0:STORE_DEPTH-1];
    reg [63:0] store_addr [0:STORE_DEPTH-1];
    reg [63:0] store_data [0:STORE_DEPTH-1];
    reg [7:0] store_mask [0:STORE_DEPTH-1];
    reg [7:0] store_wr_ptr;

    reg [63:0] trace_summary [0:TRACE_DEPTH-1];
    reg [7:0] trace_wr_ptr;
    reg [7:0] trace_rd_ptr;
    reg [8:0] trace_count;
    reg [7:0] dmem_resp_slot;

    integer idx;

    reg have_free;
    reg [7:0] free_idx;
    reg have_ready_response;
    reg [7:0] ready_response_idx;
    reg have_store_match;
    reg have_store_free;
    reg [7:0] store_match_idx;
    reg [7:0] store_free_idx;
    reg [7:0] store_target_idx;

    wire [63:0] imem_req_addr = imem_req_tdata[63:0];
    wire [31:0] imem_req_fetch_id = imem_req_tdata[95:64];
    wire [31:0] imem_req_epoch = imem_req_tdata[127:96];

    wire [31:0] dmem_req_txid = dmem_req_tdata[TXID_LSB +: 32];
    wire [7:0] dmem_req_command = dmem_req_tdata[COMMAND_LSB +: 8];
    wire [7:0] dmem_req_mask = dmem_req_tdata[MASK_LSB +: 8];
    wire [7:0] dmem_req_write_mask = dmem_req_tdata[WRITE_MASK_LSB +: 8];
    wire dmem_req_is_store = dmem_req_tdata[IS_STORE_BIT] |
        dmem_req_tdata[COMMITTED_BIT] | (dmem_req_command == 8'd1);
    wire [63:0] dmem_req_addr = dmem_req_tdata[ADDRESS_LSB +: 64];
    wire [63:0] dmem_req_data = dmem_req_tdata[DATA_LSB +: 64];
    wire [63:0] dmem_req_write_data = dmem_req_tdata[WRITE_DATA_LSB +: 64];
    wire [63:0] dmem_store_data = (dmem_req_write_data != 64'd0) ? dmem_req_write_data : dmem_req_data;
    wire [7:0] dmem_store_mask = (dmem_req_write_mask != 8'd0) ? dmem_req_write_mask : dmem_req_mask;
    wire dmem_req_in_range = (dmem_req_addr + 64'd7 < DMEM_BYTES);

    assign imem_req_tready = !imem_resp_tvalid || imem_resp_tready;
    assign dmem_req_tready = have_free;
    assign commit_trace_tready = (trace_count < TRACE_DEPTH);

    function [31:0] imem_read_word;
        input [63:0] addr;
        begin
            imem_read_word = (addr[63:2] < IMEM_WORDS) ? 32'h00000013 : 32'h00000013;
        end
    endfunction

    function [63:0] default_dmem_word;
        input [63:0] addr;
        integer byte_i;
        reg [63:0] data;
        begin
            data = 64'd0;
            for (byte_i = 0; byte_i < 8; byte_i = byte_i + 1) begin
                data[byte_i * 8 +: 8] = addr[7:0] + byte_i[7:0];
            end
            default_dmem_word = data;
        end
    endfunction

    function [63:0] apply_byte_mask;
        input [63:0] old_data;
        input [63:0] new_data;
        input [7:0] mask;
        integer byte_i;
        reg [63:0] merged;
        begin
            merged = old_data;
            for (byte_i = 0; byte_i < 8; byte_i = byte_i + 1) begin
                if (mask[byte_i]) begin
                    merged[byte_i * 8 +: 8] = new_data[byte_i * 8 +: 8];
                end
            end
            apply_byte_mask = merged;
        end
    endfunction

    function [63:0] dmem_read_qword;
        input [63:0] addr;
        integer store_i;
        reg [63:0] data;
        begin
            data = default_dmem_word(addr);
            for (store_i = 0; store_i < STORE_DEPTH; store_i = store_i + 1) begin
                if (store_valid[store_i] && store_addr[store_i][63:3] == addr[63:3]) begin
                    data = apply_byte_mask(data, store_data[store_i], store_mask[store_i]);
                end
            end
            dmem_read_qword = data;
        end
    endfunction

    function [3:0] dmem_latency;
        input [63:0] addr;
        begin
            dmem_latency = addr[4] ? 4'd2 : 4'd6;
        end
    endfunction

    function [383:0] pack_dmem_response;
        input [31:0] txid;
        input [63:0] data;
        input exception;
        input [63:0] cause;
        begin
            pack_dmem_response = 384'd0;
            pack_dmem_response[31:0] = txid;
            pack_dmem_response[127:64] = data;
            pack_dmem_response[191:128] = data;
            pack_dmem_response[192] = exception;
            pack_dmem_response[319:256] = cause;
            pack_dmem_response[383:320] = cause;
        end
    endfunction

    always @(*) begin
        have_free = 1'b0;
        free_idx = 8'd0;
        for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1) begin
            if (!pending_valid[idx] && !have_free) begin
                have_free = 1'b1;
                free_idx = idx[7:0];
            end
        end
    end

    always @(*) begin
        have_ready_response = 1'b0;
        ready_response_idx = 8'd0;
        for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1) begin
            if (pending_valid[idx] && pending_latency[idx] == 4'd0 && !have_ready_response) begin
                have_ready_response = 1'b1;
                ready_response_idx = idx[7:0];
            end
        end
    end

    always @(*) begin
        have_store_match = 1'b0;
        have_store_free = 1'b0;
        store_match_idx = 8'd0;
        store_free_idx = 8'd0;
        for (idx = 0; idx < STORE_DEPTH; idx = idx + 1) begin
            if (store_valid[idx] && store_addr[idx][63:3] == dmem_req_addr[63:3] && !have_store_match) begin
                have_store_match = 1'b1;
                store_match_idx = idx[7:0];
            end
            if (!store_valid[idx] && !have_store_free) begin
                have_store_free = 1'b1;
                store_free_idx = idx[7:0];
            end
        end
        if (have_store_match) begin
            store_target_idx = store_match_idx;
        end else if (have_store_free) begin
            store_target_idx = store_free_idx;
        end else begin
            store_target_idx = store_wr_ptr;
        end
    end

    always @(posedge ap_clk) begin
        if (!ap_rst_n) begin
            imem_resp_tvalid <= 1'b0;
            imem_resp_tdata <= 256'd0;
            dmem_resp_tvalid <= 1'b0;
            dmem_resp_tdata <= 384'd0;
            dmem_resp_slot <= 8'd0;
            imem_request_count <= 32'd0;
            dmem_read_count <= 32'd0;
            dmem_write_count <= 32'd0;
            dmem_response_count <= 32'd0;
            commit_trace_count <= 32'd0;
            commit_trace_checksum <= 32'd0;
            dmem_fault <= 1'b0;
            imem_fault <= 1'b0;
            commit_trace_overflow <= 1'b0;
            store_wr_ptr <= 8'd0;
            trace_wr_ptr <= 8'd0;
            trace_rd_ptr <= 8'd0;
            trace_count <= 9'd0;
            for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1) begin
                pending_valid[idx] <= 1'b0;
                pending_txid[idx] <= 32'd0;
                pending_data[idx] <= 64'd0;
                pending_exception[idx] <= 1'b0;
                pending_cause[idx] <= 64'd0;
                pending_latency[idx] <= 4'd0;
            end
            for (idx = 0; idx < STORE_DEPTH; idx = idx + 1) begin
                store_valid[idx] <= 1'b0;
                store_addr[idx] <= 64'd0;
                store_data[idx] <= 64'd0;
                store_mask[idx] <= 8'd0;
            end
        end else begin
            if (imem_resp_tvalid && imem_resp_tready) begin
                imem_resp_tvalid <= 1'b0;
            end

            if (imem_req_tvalid && imem_req_tready) begin
                imem_resp_tdata <= 256'd0;
                imem_resp_tdata[63:0] <= imem_req_addr;
                imem_resp_tdata[95:64] <= imem_req_fetch_id;
                imem_resp_tdata[127:96] <= imem_req_epoch;
                imem_resp_tdata[159:128] <= imem_read_word(imem_req_addr);
                imem_resp_tdata[160] <= (imem_req_addr[63:2] >= IMEM_WORDS);
                imem_resp_tdata[255:192] <= (imem_req_addr[63:2] >= IMEM_WORDS) ? 64'd1 : 64'd0;
                imem_resp_tvalid <= 1'b1;
                imem_request_count <= imem_request_count + 32'd1;
                if (imem_req_addr[63:2] >= IMEM_WORDS) begin
                    imem_fault <= 1'b1;
                end
            end

            for (idx = 0; idx < PENDING_DEPTH; idx = idx + 1) begin
                if (pending_valid[idx] && pending_latency[idx] != 4'd0) begin
                    pending_latency[idx] <= pending_latency[idx] - 4'd1;
                end
            end

            if (dmem_req_tvalid && dmem_req_tready) begin
                if (dmem_req_is_store) begin
                    store_valid[store_target_idx] <= 1'b1;
                    store_addr[store_target_idx] <= dmem_req_addr;
                    store_data[store_target_idx] <= apply_byte_mask(
                        have_store_match ? store_data[store_target_idx] : default_dmem_word(dmem_req_addr),
                        dmem_store_data,
                        dmem_store_mask
                    );
                    store_mask[store_target_idx] <= have_store_match ?
                        (store_mask[store_target_idx] | dmem_store_mask) : dmem_store_mask;
                    if (!have_store_match) begin
                        store_wr_ptr <= (store_target_idx == STORE_DEPTH - 1) ? 8'd0 : store_target_idx + 8'd1;
                    end
                    dmem_write_count <= dmem_write_count + 32'd1;
                    if (!dmem_req_in_range) begin
                        dmem_fault <= 1'b1;
                    end
                end else begin
                    pending_valid[free_idx] <= 1'b1;
                    pending_txid[free_idx] <= dmem_req_txid;
                    pending_data[free_idx] <= dmem_read_qword(dmem_req_addr);
                    pending_exception[free_idx] <= !dmem_req_in_range;
                    pending_cause[free_idx] <= !dmem_req_in_range ? 64'd5 : 64'd0;
                    pending_latency[free_idx] <= dmem_latency(dmem_req_addr);
                    dmem_read_count <= dmem_read_count + 32'd1;
                    if (!dmem_req_in_range) begin
                        dmem_fault <= 1'b1;
                    end
                end
            end

            if (dmem_resp_tvalid && dmem_resp_tready) begin
                pending_valid[dmem_resp_slot] <= 1'b0;
                dmem_resp_tvalid <= 1'b0;
                dmem_response_count <= dmem_response_count + 32'd1;
            end else if (!dmem_resp_tvalid && have_ready_response) begin
                dmem_resp_tdata <= pack_dmem_response(
                    pending_txid[ready_response_idx],
                    pending_data[ready_response_idx],
                    pending_exception[ready_response_idx],
                    pending_cause[ready_response_idx]
                );
                dmem_resp_slot <= ready_response_idx;
                dmem_resp_tvalid <= 1'b1;
            end

            if (commit_trace_tvalid && commit_trace_tready) begin
                trace_summary[trace_wr_ptr % TRACE_DEPTH] <=
                    {commit_trace_tdata[63:32], commit_trace_tdata[31:0]};
                trace_wr_ptr <= trace_wr_ptr + 8'd1;
            end

            if (trace_drain_enable && trace_count != 9'd0) begin
                commit_trace_count <= commit_trace_count + 32'd1;
                commit_trace_checksum <= commit_trace_checksum ^
                    trace_summary[trace_rd_ptr % TRACE_DEPTH][31:0] ^
                    trace_summary[trace_rd_ptr % TRACE_DEPTH][63:32];
                trace_rd_ptr <= trace_rd_ptr + 8'd1;
            end

            case ({commit_trace_tvalid && commit_trace_tready,
                   trace_drain_enable && trace_count != 9'd0})
                2'b10: trace_count <= trace_count + 9'd1;
                2'b01: trace_count <= trace_count - 9'd1;
                default: trace_count <= trace_count;
            endcase
        end
    end
endmodule
