`timescale 1 ns / 1 ps

module ppa4_memory_system_scoreboard_tb;
    localparam integer PENDING_DEPTH = 4;
    localparam integer TRACE_DEPTH = 4;

    reg ap_clk = 1'b0;
    reg ap_rst_n = 1'b0;

    reg [191:0] imem_req_tdata = 192'd0;
    reg imem_req_tvalid = 1'b0;
    wire imem_req_tready;
    wire [255:0] imem_resp_tdata;
    wire imem_resp_tvalid;
    reg imem_resp_tready = 1'b1;

    reg [319:0] dmem_req_tdata = 320'd0;
    reg dmem_req_tvalid = 1'b0;
    wire dmem_req_tready;
    wire [383:0] dmem_resp_tdata;
    wire dmem_resp_tvalid;
    reg dmem_resp_tready = 1'b1;

    reg [767:0] commit_trace_tdata = 768'd0;
    reg commit_trace_tvalid = 1'b0;
    wire commit_trace_tready;
    reg trace_drain_enable = 1'b1;

    wire [31:0] imem_request_count;
    wire [31:0] dmem_read_count;
    wire [31:0] dmem_write_count;
    wire [31:0] dmem_response_count;
    wire [31:0] commit_trace_count;
    wire [31:0] commit_trace_checksum;
    wire dmem_fault;
    wire imem_fault;
    wire commit_trace_overflow;

    integer errors = 0;
    integer cycle = 0;
    integer k;

    boom_l3_ppa4_memory_system #(
        .PENDING_DEPTH(PENDING_DEPTH),
        .TRACE_DEPTH(TRACE_DEPTH)
    ) dut (
        .ap_clk(ap_clk),
        .ap_rst_n(ap_rst_n),
        .imem_req_tdata(imem_req_tdata),
        .imem_req_tvalid(imem_req_tvalid),
        .imem_req_tready(imem_req_tready),
        .imem_resp_tdata(imem_resp_tdata),
        .imem_resp_tvalid(imem_resp_tvalid),
        .imem_resp_tready(imem_resp_tready),
        .dmem_req_tdata(dmem_req_tdata),
        .dmem_req_tvalid(dmem_req_tvalid),
        .dmem_req_tready(dmem_req_tready),
        .dmem_resp_tdata(dmem_resp_tdata),
        .dmem_resp_tvalid(dmem_resp_tvalid),
        .dmem_resp_tready(dmem_resp_tready),
        .commit_trace_tdata(commit_trace_tdata),
        .commit_trace_tvalid(commit_trace_tvalid),
        .commit_trace_tready(commit_trace_tready),
        .trace_drain_enable(trace_drain_enable),
        .imem_request_count(imem_request_count),
        .dmem_read_count(dmem_read_count),
        .dmem_write_count(dmem_write_count),
        .dmem_response_count(dmem_response_count),
        .commit_trace_count(commit_trace_count),
        .commit_trace_checksum(commit_trace_checksum),
        .dmem_fault(dmem_fault),
        .imem_fault(imem_fault),
        .commit_trace_overflow(commit_trace_overflow)
    );

    always #3.225 ap_clk = ~ap_clk;

    always @(posedge ap_clk) begin
        cycle <= cycle + 1;
        if (cycle > 5000) begin
            $display("PPA4_SCOREBOARD,FAIL,timeout");
            $finish;
        end
    end

    function [319:0] pack_dmem_req;
        input [31:0] txid;
        input [7:0] command;
        input [63:0] addr;
        input [63:0] data;
        input [7:0] mask;
        begin
            pack_dmem_req = 320'd0;
            pack_dmem_req[31:0] = txid;
            pack_dmem_req[47:40] = command;
            pack_dmem_req[55:48] = 8'd3;
            pack_dmem_req[63:56] = mask;
            pack_dmem_req[71:64] = mask;
            pack_dmem_req[88] = (command == 8'd1);
            pack_dmem_req[104] = (command == 8'd1);
            pack_dmem_req[191:128] = addr;
            pack_dmem_req[255:192] = data;
            pack_dmem_req[319:256] = data;
        end
    endfunction

    task fail;
        input [1023:0] msg;
        begin
            errors = errors + 1;
            $display("PPA4_CHECK,FAIL,%0s", msg);
        end
    endtask

    task pass;
        input [1023:0] msg;
        begin
            $display("PPA4_CHECK,PASS,%0s", msg);
        end
    endtask

    task reset_dut;
        begin
            ap_rst_n = 1'b0;
            imem_req_tvalid = 1'b0;
            dmem_req_tvalid = 1'b0;
            commit_trace_tvalid = 1'b0;
            dmem_resp_tready = 1'b1;
            imem_resp_tready = 1'b1;
            trace_drain_enable = 1'b1;
            repeat (6) @(posedge ap_clk);
            @(negedge ap_clk);
            ap_rst_n = 1'b1;
            repeat (2) @(posedge ap_clk);
        end
    endtask

    task send_imem_req;
        input [63:0] addr;
        input [31:0] fetch_id;
        input [31:0] epoch;
        begin
            @(negedge ap_clk);
            imem_req_tdata = 192'd0;
            imem_req_tdata[63:0] = addr;
            imem_req_tdata[95:64] = fetch_id;
            imem_req_tdata[127:96] = epoch;
            imem_req_tvalid = 1'b1;
            while (!imem_req_tready) @(negedge ap_clk);
            @(negedge ap_clk);
            imem_req_tvalid = 1'b0;
        end
    endtask

    task send_dmem_req;
        input [319:0] req;
        begin
            @(negedge ap_clk);
            dmem_req_tdata = req;
            dmem_req_tvalid = 1'b1;
            while (!dmem_req_tready) @(negedge ap_clk);
            @(negedge ap_clk);
            dmem_req_tvalid = 1'b0;
        end
    endtask

    task wait_dmem_resp;
        output [383:0] resp;
        begin
            while (!dmem_resp_tvalid) @(posedge ap_clk);
            resp = dmem_resp_tdata;
            @(posedge ap_clk);
        end
    endtask

    reg [383:0] resp_a;
    reg [383:0] resp_b;

    initial begin
        reset_dut();

        send_imem_req(64'h10, 32'h55, 32'haa);
        while (!imem_resp_tvalid) @(posedge ap_clk);
        if (imem_resp_tdata[63:0] != 64'h10 ||
            imem_resp_tdata[95:64] != 32'h55 ||
            imem_resp_tdata[127:96] != 32'haa ||
            imem_resp_tdata[159:128] != 32'h00000013) begin
            $display("PPA4_DEBUG,imem,addr=%h,fetch=%h,epoch=%h,inst=%h,valid=%b", imem_resp_tdata[63:0], imem_resp_tdata[95:64], imem_resp_tdata[127:96], imem_resp_tdata[159:128], imem_resp_tvalid);
            fail("imem request/response mapping");
        end else begin
            pass("imem request/response mapping");
        end
        @(posedge ap_clk);

        send_dmem_req(pack_dmem_req(32'h1, 8'd1, 64'h40, 64'hfeedfacecafebeef, 8'hff));
        send_dmem_req(pack_dmem_req(32'h2, 8'd0, 64'h40, 64'd0, 8'hff));
        wait_dmem_resp(resp_a);
        if (resp_a[31:0] != 32'h2 || resp_a[127:64] != 64'hfeedfacecafebeef) begin
            $display("PPA4_DEBUG,store_forward,txid=%h,data=%h,valid=%b", resp_a[31:0], resp_a[127:64], dmem_resp_tvalid);
            fail("store forwarding through memory image");
        end else begin
            pass("store forwarding through memory image");
        end

        send_dmem_req(pack_dmem_req(32'h11, 8'd0, 64'h00, 64'd0, 8'hff));
        send_dmem_req(pack_dmem_req(32'h22, 8'd0, 64'h10, 64'd0, 8'hff));
        wait_dmem_resp(resp_a);
        wait_dmem_resp(resp_b);
        if (resp_a[31:0] != 32'h22 || resp_b[31:0] != 32'h11) begin
            $display("PPA4_DEBUG,out_of_order,first_txid=%h,second_txid=%h", resp_a[31:0], resp_b[31:0]);
            fail("out-of-order response ownership");
        end else begin
            pass("out-of-order response ownership");
        end

        dmem_resp_tready = 1'b0;
        for (k = 0; k < PENDING_DEPTH; k = k + 1) begin
            send_dmem_req(pack_dmem_req(32'h100 + k[31:0], 8'd0, 64'h80 + k * 64'd8, 64'd0, 8'hff));
        end
        repeat (8) @(posedge ap_clk);
        if (dmem_req_tready !== 1'b0) begin
            fail("dmem backpressure when pending table is full");
        end else begin
            pass("dmem backpressure when pending table is full");
        end
        dmem_resp_tready = 1'b1;
        repeat (20) @(posedge ap_clk);

        dmem_resp_tready = 1'b0;
        send_dmem_req(pack_dmem_req(32'h333, 8'd0, 64'h00, 64'd0, 8'hff));
        repeat (2) @(posedge ap_clk);
        @(negedge ap_clk);
        ap_rst_n = 1'b0;
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk);
        ap_rst_n = 1'b1;
        dmem_resp_tready = 1'b1;
        repeat (10) @(posedge ap_clk);
        if (dmem_resp_tvalid && dmem_resp_tdata[31:0] == 32'h333) begin
            fail("reset stale response suppression");
        end else begin
            pass("reset stale response suppression");
        end

        send_dmem_req(pack_dmem_req(32'h444, 8'd0, 64'h10, 64'd0, 8'hff));
        wait_dmem_resp(resp_a);
        if (resp_a[31:0] != 32'h444) begin
            $display("PPA4_DEBUG,post_reset,txid=%h,data=%h", resp_a[31:0], resp_a[127:64]);
            fail("post-reset transaction ownership");
        end else begin
            pass("post-reset transaction ownership");
        end

        trace_drain_enable = 1'b0;
        for (k = 0; k < TRACE_DEPTH; k = k + 1) begin
            @(negedge ap_clk);
            commit_trace_tdata = {736'd0, k[31:0]};
            commit_trace_tvalid = 1'b1;
            while (!commit_trace_tready) @(negedge ap_clk);
        end
        @(negedge ap_clk);
        commit_trace_tvalid = 1'b0;
        repeat (2) @(posedge ap_clk);
        if (commit_trace_tready !== 1'b0) begin
            fail("commit trace backpressure when receiver FIFO is full");
        end else begin
            pass("commit trace backpressure when receiver FIFO is full");
        end
        trace_drain_enable = 1'b1;
        repeat (TRACE_DEPTH + 4) @(posedge ap_clk);
        if (commit_trace_tready !== 1'b1 || commit_trace_overflow) begin
            fail("commit trace drain without silent drop");
        end else begin
            pass("commit trace drain without silent drop");
        end

        if (errors == 0) begin
            $display("PPA4_SCOREBOARD,PASS,all_checks");
        end else begin
            $display("PPA4_SCOREBOARD,FAIL,%0d_errors", errors);
        end
        $finish;
    end
endmodule
