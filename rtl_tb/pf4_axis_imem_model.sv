`timescale 1ns/1ps

module pf4_axis_imem_model (
    input wire clk, input wire rst_n, input wire start_fetch,
    input wire [1:0] fault_mode,
    input wire [191:0] req_tdata, input wire req_tvalid, output wire req_tready,
    output reg [255:0] resp_tdata, output reg resp_tvalid, input wire resp_tready,
    output reg [31:0] request_count, output reg [31:0] response_count,
    output reg protocol_error, output reg fault_site_requested,
    output reg fault_sent, output reg trap_vector_requested
);
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;
    localparam [63:0] TRAP_VECTOR = RESET_VECTOR + 64'hc0;
    reg [31:0] words [0:511];
    reg pending;
    reg [1:0] delay_count;
    reg [63:0] pending_address;
    reg [31:0] pending_fetch_id, pending_epoch;
    reg held_request;
    reg [191:0] held_request_data;
    string program_file;
    integer i;

    function automatic [31:0] instruction_at(input [63:0] address);
        integer index;
        begin
            index = (address - RESET_VECTOR) >> 2;
            instruction_at = address >= RESET_VECTOR && index >= 0 && index < 512 ?
                words[index] : 32'h0000_0013;
        end
    endfunction

    function automatic is_fault_site(input [63:0] address);
        begin
            is_fault_site = (fault_mode == 1 && address == RESET_VECTOR + 4) ||
                            (fault_mode == 2 && address == RESET_VECTOR + 8);
        end
    endfunction

    assign req_tready = rst_n && start_fetch && !pending && !resp_tvalid;

    initial begin
        for (i = 0; i < 512; i = i + 1) words[i] = 32'h0000_0013;
        if (!$value$plusargs("PROGRAM=%s", program_file))
            program_file = "/tmp/boom_hls/pf4/programs/pf4_pred_nt_actual_nt.words.hex";
        $readmemh(program_file, words);
        resp_tdata = 0; resp_tvalid = 0; request_count = 0; response_count = 0;
        protocol_error = 0; fault_site_requested = 0; fault_sent = 0;
        trap_vector_requested = 0; pending = 0; delay_count = 0;
        pending_address = 0; pending_fetch_id = 0; pending_epoch = 0;
        held_request = 0; held_request_data = 0;
    end

    always @(posedge clk) begin
        if (held_request && (!req_tvalid || req_tdata !== held_request_data)) begin
            protocol_error <= 1'b1;
            $error("PF4 IMEM request changed while stalled");
        end
        held_request <= req_tvalid && !req_tready && rst_n;
        if (req_tvalid && !req_tready && rst_n) held_request_data <= req_tdata;
        if (!rst_n) begin
            pending <= 0; resp_tvalid <= 0; delay_count <= 0; held_request <= 0;
        end else begin
            if (req_tvalid && req_tready) begin
                request_count <= request_count + 1;
                pending_address <= req_tdata[63:0];
                pending_fetch_id <= req_tdata[95:64];
                pending_epoch <= req_tdata[127:96];
                delay_count <= 2;
                pending <= 1;
                if (is_fault_site(req_tdata[63:0])) fault_site_requested <= 1;
                if (req_tdata[63:0] == TRAP_VECTOR) trap_vector_requested <= 1;
            end
            if (pending) begin
                if (delay_count == 0) begin
                    if (is_fault_site(pending_address) && !fault_sent) begin
                        resp_tdata <= {31'd0, 64'd1, 1'b1, instruction_at(pending_address),
                                       pending_epoch, pending_fetch_id, pending_address};
                        fault_sent <= 1;
                    end else begin
                        resp_tdata <= {31'd0, 64'd0, 1'b0, instruction_at(pending_address),
                                       pending_epoch, pending_fetch_id, pending_address};
                    end
                    resp_tvalid <= 1; pending <= 0;
                end else delay_count <= delay_count - 1;
            end
            if (resp_tvalid && resp_tready) begin
                response_count <= response_count + 1;
                resp_tvalid <= 0;
            end
        end
    end
endmodule
