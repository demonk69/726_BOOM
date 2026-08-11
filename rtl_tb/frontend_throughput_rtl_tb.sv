`timescale 1ns/1ps

module frontend_throughput_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg imem_req_out_full_n = 1;
    reg [224:0] imem_resp_in_dout = 0;
    reg imem_resp_in_empty_n = 0;
    reg [63:0] seed = 0;
    wire ap_done, ap_idle, ap_ready;
    wire [128:0] imem_req_out_din;
    wire imem_req_out_write, imem_resp_in_read;
    wire [63:0] observable;
    wire observable_ap_vld;

    integer cycle = 0;
    integer request_count = 0;
    integer response_count = 0;
    integer latency_mode = 0;
    integer delay_count = 0;
    integer pending = 0;
    integer last_request_cycle = -1;
    integer interval_min = 1000000;
    integer interval_max = 0;
    reg [224:0] pending_response = 0;
    reg [63:0] expected_address = 64'h10040;
    reg [31:0] expected_fetch_id = 0;

    synth_frontend_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .imem_req_out_din(imem_req_out_din),
        .imem_req_out_full_n(imem_req_out_full_n),
        .imem_req_out_write(imem_req_out_write),
        .imem_resp_in_dout(imem_resp_in_dout),
        .imem_resp_in_empty_n(imem_resp_in_empty_n),
        .imem_resp_in_read(imem_resp_in_read),
        .seed(seed), .observable(observable), .observable_ap_vld(observable_ap_vld)
    );

    always #5 ap_clk = ~ap_clk;

    always @(posedge ap_clk) begin
        cycle = cycle + 1;
        if (imem_resp_in_read && imem_resp_in_empty_n) begin
            pending = 0;
            response_count = response_count + 1;
        end
        if (imem_req_out_write && imem_req_out_full_n) begin
            if (pending && !(imem_resp_in_read && imem_resp_in_empty_n))
                $fatal(1, "more than one outstanding request");
            if (imem_req_out_din[63:0] !== expected_address ||
                imem_req_out_din[95:64] !== expected_fetch_id ||
                imem_req_out_din[127:96] !== 0)
                $fatal(1, "request identity sequence changed count=%0d", request_count);
            if (last_request_cycle >= 0 && request_count >= 4) begin
                if (cycle-last_request_cycle < interval_min) interval_min = cycle-last_request_cycle;
                if (cycle-last_request_cycle > interval_max) interval_max = cycle-last_request_cycle;
            end
            last_request_cycle = cycle;
            pending_response = {64'd0, 1'b0, 32'h00000013,
                                imem_req_out_din[127:96], imem_req_out_din[95:64],
                                imem_req_out_din[63:0]};
            pending = 1;
            if (latency_mode == 4) delay_count = request_count % 4;
            else delay_count = latency_mode;
            request_count = request_count + 1;
            expected_address = expected_address + 4;
            expected_fetch_id = expected_fetch_id + 1;
        end
        if (request_count >= 40) begin
            $display("THROUGHPUT_PASS,latency=%0d,requests=%0d,responses=%0d,interval_min=%0d,interval_max=%0d",
                     latency_mode, request_count, response_count, interval_min, interval_max);
            $finish;
        end
        if (cycle > 2000) $fatal(1, "throughput timeout");
    end

    always @(negedge ap_clk) begin
        if (ap_rst) begin
            imem_resp_in_empty_n = 0;
        end else if (pending) begin
            if (delay_count > 0) begin
                delay_count = delay_count - 1;
                imem_resp_in_empty_n = 0;
            end else begin
                imem_resp_in_dout = pending_response;
                imem_resp_in_empty_n = 1;
            end
        end else begin
            imem_resp_in_empty_n = 0;
        end
    end

    initial begin
        if (!$value$plusargs("LATENCY=%d", latency_mode)) latency_mode = 0;
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0; ap_start = 1;
    end
endmodule
