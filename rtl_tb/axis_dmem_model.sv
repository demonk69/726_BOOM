`timescale 1ns/1ps

module axis_dmem_model (
    input  wire         clk,
    input  wire         rst_n,
    input  wire [7:0]   scenario_code,
    input  wire [319:0] req_tdata,
    input  wire         req_tvalid,
    output wire         req_tready,
    output reg  [383:0] resp_tdata,
    output reg          resp_tvalid,
    input  wire         resp_tready,
    output reg  [31:0]  request_count,
    output reg  [31:0]  response_count,
    output reg  [31:0]  load_count,
    output reg  [31:0]  store_count,
    output reg          tohost_seen,
    output reg  [63:0]  tohost_value,
    output reg          protocol_error,
    output wire         pending_visible
);
    localparam [63:0] ENTRY_PC   = 64'h0000_0000_8000_0000;
    localparam [63:0] TOHOST_ADDR = 64'h0000_0000_8000_0080;

    reg [7:0] memory [0:4095];
    integer i;
    integer cycle_count;
    integer delay_count;
    reg pending;
    reg [31:0] pending_tx;
    reg [63:0] pending_address;
    reg actual_after_stale;
    reg [31:0] actual_tx;
    reg [63:0] actual_address;
    reg stale_after_reset;
    reg [31:0] stale_tx;
    reg [63:0] stale_address;
    reg was_in_reset;
    reg held_request;
    reg [319:0] held_request_data;

    wire [31:0] request_tx = req_tdata[31:0];
    wire [7:0] request_command = req_tdata[47:40];
    wire [7:0] request_size = req_tdata[55:48];
    wire [7:0] request_mask = req_tdata[63:56];
    wire [7:0] request_write_mask = req_tdata[71:64];
    wire request_is_store = req_tdata[88] || request_command == 8'd1;
    wire request_committed = req_tdata[104];
    wire [63:0] request_address = req_tdata[191:128];
    wire [63:0] request_data = req_tdata[255:192];
    wire [63:0] request_write_data = req_tdata[319:256];

    function automatic [63:0] read_beat(input [63:0] address);
        integer offset;
        integer byte_index;
        begin
            read_beat = 64'd0;
            offset = (address & ~64'h7) - ENTRY_PC;
            for (byte_index = 0; byte_index < 8; byte_index = byte_index + 1) begin
                if (offset + byte_index >= 0 && offset + byte_index < 4096)
                    read_beat[byte_index * 8 +: 8] = memory[offset + byte_index];
            end
        end
    endfunction

    function automatic integer response_delay(input [7:0] code);
        begin
            case (code)
                8'd33: response_delay = 4;                         // D3
                8'd34: response_delay = (cycle_count % 7) + 1;     // D4
                8'd4:  response_delay = 16;                        // R4
                8'd37: response_delay = 16;                        // D7
                8'd41: response_delay = 16;                        // P1
                default: response_delay = 0;
            endcase
        end
    endfunction

    function automatic preserve_across_reset(input [7:0] code);
        begin
            preserve_across_reset = (code == 8'd4 || code == 8'd37 || code == 8'd41);
        end
    endfunction

    wire directed_stall_scenario =
        scenario_code == 8'd5 || scenario_code == 8'd31 || scenario_code == 8'd32 ||
        scenario_code == 8'd38 || scenario_code == 8'd42 || scenario_code == 8'd46;
    wire directed_stall = directed_stall_scenario && (cycle_count[1:0] != 2'b11);
    assign req_tready = rst_n && !pending && !resp_tvalid && !stale_after_reset && !directed_stall;
    assign pending_visible = pending || resp_tvalid || stale_after_reset;

    initial begin
        for (i = 0; i < 4096; i = i + 1) memory[i] = 8'd0;
        resp_tdata = 384'd0;
        resp_tvalid = 1'b0;
        request_count = 0;
        response_count = 0;
        load_count = 0;
        store_count = 0;
        tohost_seen = 1'b0;
        tohost_value = 0;
        protocol_error = 1'b0;
        cycle_count = 0;
        delay_count = 0;
        pending = 1'b0;
        pending_tx = 0;
        pending_address = 0;
        actual_after_stale = 1'b0;
        actual_tx = 0;
        actual_address = 0;
        stale_after_reset = 1'b0;
        stale_tx = 0;
        stale_address = 0;
        was_in_reset = 1'b1;
        held_request = 1'b0;
        held_request_data = 0;
    end

    always @(posedge clk) begin : DMEM_SEQUENTIAL
        integer offset;
        integer byte_index;
        reg [7:0] effective_mask;
        reg [63:0] effective_data;

        cycle_count <= cycle_count + 1;

        if (held_request && (!req_tvalid || req_tdata !== held_request_data)) begin
            protocol_error <= 1'b1;
            $error("DMEM request TVALID/TDATA changed while TREADY was low");
        end
        held_request <= req_tvalid && !req_tready && rst_n;
        if (req_tvalid && !req_tready && rst_n) held_request_data <= req_tdata;

        if (!rst_n) begin
            if (!was_in_reset && preserve_across_reset(scenario_code) && (pending || resp_tvalid)) begin
                stale_after_reset <= 1'b1;
                if (pending) begin
                    stale_tx <= pending_tx;
                    stale_address <= pending_address;
                end else begin
                    stale_tx <= resp_tdata[31:0];
                    stale_address <= pending_address;
                end
            end
            pending <= 1'b0;
            resp_tvalid <= 1'b0;
            delay_count <= 0;
            was_in_reset <= 1'b1;
            held_request <= 1'b0;
        end else begin
            was_in_reset <= 1'b0;

            if (stale_after_reset && !resp_tvalid) begin
                resp_tdata <= {64'd0, 64'd0, 63'd0, 1'b0, read_beat(stale_address),
                               read_beat(stale_address), 32'd0, stale_tx};
                resp_tvalid <= 1'b1;
            end

            if (req_tvalid && req_tready) begin
                request_count <= request_count + 1;
                if (request_is_store) begin
                    store_count <= store_count + 1;
                    effective_mask = request_write_mask != 0 ? request_write_mask : request_mask;
                    effective_data = request_write_data != 0 ? request_write_data : request_data;
                    offset = request_address - ENTRY_PC;
                    for (byte_index = 0; byte_index < 8; byte_index = byte_index + 1) begin
                        if (effective_mask[byte_index] && offset + byte_index >= 0 && offset + byte_index < 4096)
                            memory[offset + byte_index] <= effective_data[byte_index * 8 +: 8];
                    end
                    if (request_address == TOHOST_ADDR && request_committed) begin
                        tohost_seen <= 1'b1;
                        tohost_value <= effective_data;
                    end
                end else begin
                    load_count <= load_count + 1;
                    pending <= 1'b1;
                    pending_tx <= (scenario_code == 8'd35) ? request_tx + 32'd999 : request_tx;
                    pending_address <= request_address;
                    delay_count <= response_delay(scenario_code);
                    if (scenario_code == 8'd35) begin
                        actual_after_stale <= 1'b1;
                        actual_tx <= request_tx;
                        actual_address <= request_address;
                    end
                end
            end

            if (pending) begin
                if (delay_count == 0) begin
                    resp_tdata <= {64'd0, 64'd0, 63'd0, 1'b0, read_beat(pending_address),
                                   read_beat(pending_address), 32'd0, pending_tx};
                    resp_tvalid <= 1'b1;
                    pending <= 1'b0;
                end else begin
                    delay_count <= delay_count - 1;
                end
            end

            if (resp_tvalid && resp_tready) begin
                response_count <= response_count + 1;
                resp_tvalid <= 1'b0;
                if (stale_after_reset) begin
                    stale_after_reset <= 1'b0;
                end else if (actual_after_stale) begin
                    actual_after_stale <= 1'b0;
                    pending <= 1'b1;
                    pending_tx <= actual_tx;
                    pending_address <= actual_address;
                    delay_count <= 2;
                end
            end
        end
    end
endmodule
