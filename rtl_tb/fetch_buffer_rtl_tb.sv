`timescale 1ns/1ps

module fetch_buffer_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg runtime_reset = 0;
    reg flush = 0;
    reg dequeue_ready = 0;
    reg packet_valid = 0;
    reg [7:0] valid_mask = 0;
    reg [63:0] pc0 = 0, pc1 = 0, pc2 = 0, pc3 = 0;
    reg [31:0] instruction0 = 0, instruction1 = 0, instruction2 = 0, instruction3 = 0;
    reg [31:0] fetch_id0 = 0, fetch_id1 = 0, fetch_id2 = 0, fetch_id3 = 0;
    reg [63:0] cause0 = 0, cause1 = 0, cause2 = 0, cause3 = 0;
    reg is_rvc0 = 0, is_rvc1 = 0, is_rvc2 = 0, is_rvc3 = 0;
    reg exception0 = 0, exception1 = 0, exception2 = 0, exception3 = 0;

    wire ap_done, ap_idle, ap_ready;
    wire enqueue_ready, enqueue_fire, dequeue_valid, dequeue_fire;
    wire [63:0] dequeue_pc, dequeue_cause;
    wire [31:0] dequeue_instruction, dequeue_fetch_id;
    wire dequeue_is_rvc, dequeue_exception, full, empty;
    wire [7:0] count;
    integer pass_count = 0;
    integer id;

    synth_fetch_buffer_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .runtime_reset(runtime_reset), .flush(flush), .dequeue_ready(dequeue_ready),
        .packet_valid(packet_valid), .valid_mask(valid_mask),
        .pc0(pc0), .instruction0(instruction0), .fetch_id0(fetch_id0), .cause0(cause0),
        .is_rvc0(is_rvc0), .exception0(exception0),
        .pc1(pc1), .instruction1(instruction1), .fetch_id1(fetch_id1), .cause1(cause1),
        .is_rvc1(is_rvc1), .exception1(exception1),
        .pc2(pc2), .instruction2(instruction2), .fetch_id2(fetch_id2), .cause2(cause2),
        .is_rvc2(is_rvc2), .exception2(exception2),
        .pc3(pc3), .instruction3(instruction3), .fetch_id3(fetch_id3), .cause3(cause3),
        .is_rvc3(is_rvc3), .exception3(exception3),
        .enqueue_ready(enqueue_ready), .enqueue_fire(enqueue_fire),
        .dequeue_valid(dequeue_valid), .dequeue_fire(dequeue_fire),
        .dequeue_pc(dequeue_pc), .dequeue_instruction(dequeue_instruction),
        .dequeue_fetch_id(dequeue_fetch_id), .dequeue_cause(dequeue_cause),
        .dequeue_is_rvc(dequeue_is_rvc), .dequeue_exception(dequeue_exception),
        .full(full), .empty(empty), .count(count)
    );

    always #5 ap_clk = ~ap_clk;

    task fail(input [8*64-1:0] name, input [8*128-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "GATE5_3_B1_FETCH_BUFFER_RTL_FAIL case=%0s", name);
        end
    endtask

    task pass(input [8*64-1:0] name, input [8*128-1:0] requirement);
        begin
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task set_lane(input integer lane, input integer value);
        begin
            case (lane)
                0: begin pc0 = 64'h1000 + value * 2; instruction0 = 32'h13 ^ (value << 7);
                         fetch_id0 = value; cause0 = 64'h80 + value;
                         is_rvc0 = value[0]; exception0 = (value % 7) == 0; end
                1: begin pc1 = 64'h1000 + value * 2; instruction1 = 32'h13 ^ (value << 7);
                         fetch_id1 = value; cause1 = 64'h80 + value;
                         is_rvc1 = value[0]; exception1 = (value % 7) == 0; end
                2: begin pc2 = 64'h1000 + value * 2; instruction2 = 32'h13 ^ (value << 7);
                         fetch_id2 = value; cause2 = 64'h80 + value;
                         is_rvc2 = value[0]; exception2 = (value % 7) == 0; end
                3: begin pc3 = 64'h1000 + value * 2; instruction3 = 32'h13 ^ (value << 7);
                         fetch_id3 = value; cause3 = 64'h80 + value;
                         is_rvc3 = value[0]; exception3 = (value % 7) == 0; end
            endcase
        end
    endtask

    task set_packet(input [7:0] mask, input integer base);
        begin
            packet_valid = 1;
            valid_mask = mask;
            set_lane(0, base);
            set_lane(1, base + 1);
            set_lane(2, base + 2);
            set_lane(3, base + 3);
        end
    endtask

    task invoke;
        integer guard;
        begin
            @(negedge ap_clk); ap_start = 1;
            @(negedge ap_clk); ap_start = 0;
            guard = 0;
            while (ap_done !== 1'b1 && guard < 100) begin
                @(negedge ap_clk);
                guard = guard + 1;
            end
            #1;
            if (ap_done !== 1'b1) fail("generated_control_timeout", "ap_done did not arrive");
            @(posedge ap_clk); #1;
        end
    endtask

    task idle_inputs;
        begin packet_valid = 0; valid_mask = 0; dequeue_ready = 0; flush = 0; end
    endtask

    task expect_item(input integer expected);
        begin
            if (!dequeue_valid || !dequeue_fire || dequeue_fetch_id !== expected ||
                dequeue_pc !== 64'h1000 + expected * 2 ||
                dequeue_instruction !== (32'h13 ^ (expected << 7)) ||
                dequeue_cause !== 64'h80 + expected || dequeue_is_rvc !== expected[0] ||
                dequeue_exception !== ((expected % 7) == 0))
                fail("payload_order", "dequeued payload or metadata mismatch");
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0; runtime_reset = 1;
        invoke();
        runtime_reset = 0;
        if (!empty || count != 0) fail("runtime_reset", "reset did not empty queue");
        pass("runtime_reset", "runtime reset clears control state");

        set_packet(8'ha, 10); invoke();
        if (!enqueue_fire || count != 2) fail("sparse_mask", "mask 1010 did not enqueue two");
        pass("sparse_mask", "partial packet compacts valid lanes");
        idle_inputs(); dequeue_ready = 1; invoke(); expect_item(11);
        invoke(); expect_item(13);
        if (!empty) fail("one_wide_drain", "partial packet did not drain");
        pass("one_wide_drain", "one instruction drains per invocation");

        idle_inputs(); set_packet(8'hf, 0); invoke();
        set_packet(8'hf, 4); invoke();
        if (!full || count != 8) fail("full", "two packets did not fill depth eight");
        pass("full", "two four-wide packets fill eight entries");
        set_packet(8'h1, 100); invoke();
        if (enqueue_ready || enqueue_fire || count != 8) fail("backpressure", "full queue accepted packet");
        pass("backpressure", "full queue atomically backpressures enqueue");

        dequeue_ready = 1; invoke(); expect_item(0);
        if (!enqueue_fire || !full) fail("capacity_reuse", "same-cycle slot was not reused");
        pass("capacity_reuse", "dequeue capacity permits same-cycle enqueue");
        idle_inputs(); dequeue_ready = 1;
        for (id = 1; id < 8; id = id + 1) begin invoke(); expect_item(id); end
        invoke(); expect_item(100);
        if (!empty) fail("fifo_order", "replacement did not drain last");
        pass("fifo_order", "FIFO order survives full replacement");

        idle_inputs();
        for (id = 0; id < 8; id = id + 1) begin set_packet(8'h1, id); invoke(); end
        idle_inputs(); dequeue_ready = 1;
        for (id = 0; id < 4; id = id + 1) begin invoke(); expect_item(id); end
        dequeue_ready = 0;
        for (id = 8; id < 12; id = id + 1) begin set_packet(8'h1, id); invoke(); end
        idle_inputs(); dequeue_ready = 1;
        for (id = 4; id < 12; id = id + 1) begin invoke(); expect_item(id); end
        pass("head_tail_wrap", "head and tail wrap preserve FIFO order");

        idle_inputs(); set_packet(8'hf, 200); invoke();
        set_packet(8'h1, 300); dequeue_ready = 1; flush = 1; invoke();
        if (dequeue_valid || dequeue_fire || enqueue_ready || enqueue_fire || !empty || count != 0)
            fail("flush_priority", "flush did not dominate traffic");
        pass("flush_priority", "flush removes all entries and rejects concurrent traffic");

        idle_inputs(); set_packet(0, 400); invoke();
        if (!enqueue_fire || !empty) fail("zero_mask", "zero mask packet changed occupancy");
        pass("zero_mask", "valid zero-mask packet is accepted no-op");

        $display("GATE5_3_B1_FETCH_BUFFER_RTL_PASS cases=%0d", pass_count);
        $finish;
    end
endmodule
