`timescale 1ns/1ps

module axis_imem_model (
    input  wire         clk,
    input  wire         rst_n,
    input  wire [7:0]   scenario_code,
    input  wire [127:0] req_tdata,
    input  wire         req_tvalid,
    output wire         req_tready,
    output reg  [255:0] resp_tdata,
    output reg          resp_tvalid,
    input  wire         resp_tready,
    output reg  [31:0]  request_count,
    output reg  [31:0]  response_count,
    output reg          protocol_error,
    output wire         pending_visible
);
    localparam [63:0] RESET_VECTOR = 64'h0000_0000_0001_0040;
    localparam [63:0] ENTRY_PC     = 64'h0000_0000_8000_0000;

    reg [63:0] program_words [0:255];
    string program_file;
    integer i;
    integer delay_count;
    integer cycle_count;
    reg pending;
    reg [63:0] pending_address;
    reg [31:0] pending_fetch_id;
    reg stale_after_reset;
    reg [63:0] stale_address;
    reg [31:0] stale_fetch_id;
    reg was_in_reset;
    reg inject_stale_response;
    reg stale_redirect_injected;
    reg held_request;
    reg [127:0] held_request_data;

    function automatic [31:0] instruction_at(input [63:0] address);
        integer word_index;
        integer line_index;
        begin
            case (address)
                RESET_VECTOR:          instruction_at = 32'h0010_0293; // addi x5,x0,1
                RESET_VECTOR + 64'd4:  instruction_at = 32'h01f2_9293; // slli x5,x5,31
                RESET_VECTOR + 64'd8:  instruction_at = 32'h0002_8067; // jalr x0,0(x5)
                default: begin
                    if (address >= ENTRY_PC && address < ENTRY_PC + 64'd2048) begin
                        word_index = (address - ENTRY_PC) >> 2;
                        line_index = word_index >> 1;
                        if (word_index[0])
                            instruction_at = program_words[line_index][63:32];
                        else
                            instruction_at = program_words[line_index][31:0];
                    end else begin
                        instruction_at = 32'h0000_006f; // finite tests should never execute here
                    end
                end
            endcase
        end
    endfunction

    function automatic integer response_delay(input [7:0] code);
        begin
            case (code)
                8'd22: response_delay = 1;                         // I2
                8'd23: response_delay = 4;                         // I3
                8'd24: response_delay = (cycle_count % 5) + 1;     // I4
                8'd1:  response_delay = 12;                        // R1
                8'd26: response_delay = 12;                        // I6
                default: response_delay = 0;
            endcase
        end
    endfunction

    function automatic preserve_across_reset(input [7:0] code);
        begin
            preserve_across_reset = (code == 8'd1 || code == 8'd26);
        end
    endfunction

    wire request_stalled = (scenario_code == 8'd21) && (cycle_count[1:0] != 2'b11);
    assign req_tready = rst_n && !pending && !resp_tvalid && !stale_after_reset && !request_stalled;
    assign pending_visible = pending || resp_tvalid || stale_after_reset;

    initial begin
        for (i = 0; i < 256; i = i + 1) program_words[i] = 64'd0;
        if (!$value$plusargs("PROGRAM=%s", program_file))
            program_file = "tb/programs/boom_reference/build/independent_alu.hex";
        $display("GATE3_8_IMEM program=%s", program_file);
        $readmemh(program_file, program_words);
        resp_tdata = 256'd0;
        resp_tvalid = 1'b0;
        request_count = 0;
        response_count = 0;
        protocol_error = 1'b0;
        pending = 1'b0;
        delay_count = 0;
        cycle_count = 0;
        stale_after_reset = 1'b0;
        stale_address = 0;
        stale_fetch_id = 0;
        inject_stale_response = 1'b0;
        stale_redirect_injected = 1'b0;
        was_in_reset = 1'b1;
        held_request = 1'b0;
        held_request_data = 0;
    end

    always @(posedge clk) begin
        cycle_count <= cycle_count + 1;

        if (held_request && (!req_tvalid || req_tdata !== held_request_data)) begin
            protocol_error <= 1'b1;
            $error("IMEM request TVALID/TDATA changed while TREADY was low");
        end
        held_request <= req_tvalid && !req_tready && rst_n;
        if (req_tvalid && !req_tready && rst_n) held_request_data <= req_tdata;

        if (!rst_n) begin
            if (!was_in_reset && preserve_across_reset(scenario_code) && (pending || resp_tvalid)) begin
                stale_after_reset <= 1'b1;
                if (pending) begin
                    stale_address <= pending_address;
                    stale_fetch_id <= pending_fetch_id;
                end else begin
                    stale_address <= resp_tdata[63:0];
                    stale_fetch_id <= resp_tdata[95:64];
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
                resp_tdata <= {64'd0, 63'd0, 1'b0, instruction_at(stale_address), stale_fetch_id, stale_address};
                resp_tvalid <= 1'b1;
            end

            if (inject_stale_response && !resp_tvalid && !pending) begin
                resp_tdata <= {64'd0, 63'd0, 1'b0, instruction_at(stale_address), stale_fetch_id, stale_address};
                resp_tvalid <= 1'b1;
                inject_stale_response <= 1'b0;
            end

            if (req_tvalid && req_tready) begin
                request_count <= request_count + 1;
                pending_address <= req_tdata[63:0];
                pending_fetch_id <= req_tdata[95:64];
                delay_count <= response_delay(scenario_code);
                pending <= 1'b1;
            end

            if (pending) begin
                if (delay_count == 0) begin
                    resp_tdata <= {64'd0, 63'd0, 1'b0, instruction_at(pending_address), pending_fetch_id, pending_address};
                    resp_tvalid <= 1'b1;
                    pending <= 1'b0;
                end else begin
                    delay_count <= delay_count - 1;
                end
            end

            if (resp_tvalid && resp_tready) begin
                response_count <= response_count + 1;
                resp_tvalid <= 1'b0;
                if (stale_after_reset) stale_after_reset <= 1'b0;
                if ((scenario_code == 8'd25 || scenario_code == 8'd47) &&
                    !stale_redirect_injected && request_count >= 5) begin
                    stale_address <= resp_tdata[63:0];
                    stale_fetch_id <= resp_tdata[95:64];
                    inject_stale_response <= 1'b1;
                    stale_redirect_injected <= 1'b1;
                end
            end
        end
    end
endmodule
