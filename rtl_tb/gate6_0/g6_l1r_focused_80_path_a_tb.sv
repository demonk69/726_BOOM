`timescale 1ns/1ps

module g6_l1r_focused_80_path_a_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    wire ap_done, ap_idle, ap_ready;
    wire [63:0] obs0, obs1, obs2, obs3, obs4, obs5, obs6, obs7;
    reg [63:0] expected [0:7];
    integer case_id;
    integer cycles;
    integer i;

    reg [7:0] queue_kind, tail, initial_count, allocations, mask;
    reg [15:0] generation;
    reg [31:0] allocation_base, transaction;
    reg [63:0] address;

    reg [7:0] initial_slot, phase_enables;

    reg [7:0] rob_idx, slot, pending_rob, owner_rob, flags, pdst;
    reg [7:0] memory_mask, sequence_mode;
    reg [15:0] pending_generation;
    reg [31:0] live_allocation, pending_allocation, owner_allocation;
    reg [31:0] pending_transaction, owner_transaction;
    reg [31:0] first_response_transaction, second_response_transaction;
    reg [63:0] data;

    reg [7:0] tag, lq_slot, sq_slot, queue_valids;
    reg [7:0] lq_branch_mask, sq_branch_mask, mispredict;

    reg [7:0] head, store_rob, load_rob, store_mask, load_mask;
    reg [63:0] store_address, load_address;

`ifdef F80_QUEUE_ALLOCATE
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .queue_kind(queue_kind), .tail(tail), .initial_count(initial_count),
        .allocations(allocations), .generation(generation),
        .allocation_base(allocation_base), .mask(mask), .address(address),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3),
        .obs4(obs4), .obs5(obs5), .obs6(obs6), .obs7(obs7));
`elsif F80_QUEUE_LIFECYCLE
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .queue_kind(queue_kind), .initial_count(initial_count),
        .initial_slot(initial_slot), .generation(generation),
        .phase_enables(phase_enables), .allocation_base(allocation_base),
        .transaction_r(transaction), .mask(mask),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3),
        .obs4(obs4), .obs5(obs5), .obs6(obs6), .obs7(obs7));
`elsif F80_LOAD_RESPONSE
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .rob_idx(rob_idx), .slot(slot), .generation(generation),
        .live_allocation(live_allocation), .pending_allocation(pending_allocation),
        .owner_allocation(owner_allocation), .transaction_r(transaction),
        .pending_transaction(pending_transaction),
        .owner_transaction(owner_transaction),
        .first_response_transaction(first_response_transaction),
        .second_response_transaction(second_response_transaction),
        .pending_generation(pending_generation), .pending_rob(pending_rob),
        .owner_rob(owner_rob), .flags(flags), .pdst(pdst),
        .memory_mask(memory_mask), .sequence_mode(sequence_mode),
        .data(data), .address(address),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3),
        .obs4(obs4), .obs5(obs5), .obs6(obs6), .obs7(obs7));
`elsif F80_BRANCH_RECOVERY
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .tag(tag), .lq_slot(lq_slot), .sq_slot(sq_slot),
        .generation(generation), .allocation_base(allocation_base),
        .transaction_r(transaction), .queue_valids(queue_valids),
        .lq_branch_mask(lq_branch_mask), .sq_branch_mask(sq_branch_mask),
        .memory_mask(memory_mask), .mispredict(mispredict),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3),
        .obs4(obs4), .obs5(obs5), .obs6(obs6), .obs7(obs7));
`elsif F80_GLOBAL_FLUSH
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .lq_slot(lq_slot), .sq_slot(sq_slot), .generation(generation),
        .allocation(allocation_base), .transaction_r(transaction),
        .branch_mask(mask),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3),
        .obs4(obs4), .obs5(obs5), .obs6(obs6), .obs7(obs7));
`elsif F80_OLDER_STORE
    `DUT_TOP dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle),
        .ap_ready(ap_ready), .head(head), .store_rob(store_rob),
        .load_rob(load_rob), .allocation_base(allocation_base),
        .store_address(store_address), .load_address(load_address),
        .store_mask(store_mask), .load_mask(load_mask),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3),
        .obs4(obs4), .obs5(obs5), .obs6(obs6), .obs7(obs7));
`else
    initial $fatal(1, "missing PATH_A family define");
`endif

    always #5 ap_clk = ~ap_clk;

    function automatic [15:0] variant_generation(input integer index);
        case (index % 6)
            0: variant_generation = 16'h0000;
            1: variant_generation = 16'h0001;
            2: variant_generation = 16'h7fff;
            3: variant_generation = 16'hfffd;
            4: variant_generation = 16'hfffe;
            default: variant_generation = 16'hffff;
        endcase
    endfunction

    function automatic [7:0] variant_mask(input integer index);
        case (index)
            0: variant_mask=8'h01; 1: variant_mask=8'h02;
            2: variant_mask=8'h04; 3: variant_mask=8'h08;
            4: variant_mask=8'h10; 5: variant_mask=8'h20;
            6: variant_mask=8'h40; 7: variant_mask=8'h80;
            8: variant_mask=8'h3f; 9: variant_mask=8'h1f;
            10: variant_mask=8'h0f; 11: variant_mask=8'h07;
            12: variant_mask=8'h03; 13: variant_mask=8'h01;
            14: variant_mask=8'h00; 15: variant_mask=8'h00;
            16: variant_mask=8'hb5; 17: variant_mask=8'hb4;
            18: variant_mask=8'hb7; 19: variant_mask=8'hb6;
            20: variant_mask=8'hb1; 21: variant_mask=8'hb0;
            22: variant_mask=8'hb3; 23: variant_mask=8'hb2;
            24: variant_mask=8'hbd; 25: variant_mask=8'hbc;
            26: variant_mask=8'hbf; 27: variant_mask=8'hbe;
            28: variant_mask=8'hb9; default: variant_mask=8'hb8;
        endcase
    endfunction

    function automatic [7:0] first_set_bit(input [7:0] value);
        integer bit_index;
        begin
            first_set_bit = 0;
            for (bit_index = 7; bit_index >= 0; bit_index = bit_index - 1)
                if (value[bit_index]) first_set_bit = bit_index;
        end
    endfunction

    task automatic configure_load_common;
        input integer id;
        begin
            rob_idx = 5;
            slot = 2;
            generation = 7;
            live_allocation = 32'h30000000 + id * 32'h100;
            pending_allocation = live_allocation;
            owner_allocation = live_allocation;
            transaction = 32'h1000 + id;
            pending_transaction = transaction;
            owner_transaction = transaction;
            first_response_transaction = transaction;
            second_response_transaction = transaction + 1;
            pending_generation = generation;
            pending_rob = rob_idx;
            owner_rob = rob_idx;
            flags = 8'h77;
            pdst = 5;
            memory_mask = 8'hff;
            sequence_mode = 0;
            data = 64'h1122334455660000 + id;
            address = 64'h4000 + id * 8;
        end
    endtask

    task automatic configure_variant;
        input integer id;
        integer index;
        begin
            index = id - 50;
            slot = (index + 2) % 8;
            lq_slot = slot;
            sq_slot = slot;
            initial_slot = slot;
            generation = variant_generation(index);
            mask = variant_mask(index);
            memory_mask = mask;
            transaction = index * 32'h00010101 + 1;
            case (index % 6)
                0, 2: begin
                    configure_load_common(id);
                    slot = (index + 2) % 8;
                    generation = variant_generation(index);
                    memory_mask = variant_mask(index);
                    transaction = index * 32'h00010101 + 1;
                    pending_transaction = transaction;
                    owner_transaction = transaction;
                    first_response_transaction = transaction;
                    second_response_transaction = transaction + 1;
                    pending_generation = generation;
                    if ((index % 6) == 2) begin
                        first_response_transaction = transaction + 1;
                        second_response_transaction = transaction;
                    end
                end
                1: begin
                    queue_kind = 1;
                    initial_count = slot;
                    phase_enables = 8'h01;
                    allocation_base = 32'h20000000 + id * 32'h100;
                end
                3: begin
                    allocation_base = 32'h50000000 + id * 32'h100;
                end
                4: begin
                    tag = first_set_bit(mask);
                    allocation_base = 32'h40000000 + id * 32'h100;
                    queue_valids = 1;
                    lq_branch_mask = mask;
                    sq_branch_mask = 0;
                    mispredict = 1;
                end
                5: begin
                    queue_kind = 0;
                    initial_count = 8;
                    phase_enables = 8'h0c;
                    allocation_base = 32'h20000000 + id * 32'h100;
                end
            endcase
        end
    endtask

    task automatic configure_case;
        input integer id;
        begin
            queue_kind=0; tail=0; initial_count=0; allocations=0; mask=8'hff;
            generation=0; allocation_base=0; transaction=0; address=0;
            initial_slot=0; phase_enables=0; rob_idx=0; slot=0;
            pending_rob=0; owner_rob=0; flags=0; pdst=0; memory_mask=0;
            sequence_mode=0; pending_generation=0; live_allocation=0;
            pending_allocation=0; owner_allocation=0; pending_transaction=0;
            owner_transaction=0; first_response_transaction=0;
            second_response_transaction=0; data=0; tag=0; lq_slot=0; sq_slot=0;
            queue_valids=0; lq_branch_mask=0; sq_branch_mask=0; mispredict=0;
            head=0; store_rob=1; load_rob=2; store_mask=0; load_mask=0;
            store_address=0; load_address=0;
            if (id >= 50) begin
                configure_variant(id);
            end else begin
                case (id)
                    0: begin queue_kind=0; allocations=0; end
                    1: begin queue_kind=0; allocations=1; end
                    2: begin queue_kind=1; allocations=1; end
                    3: begin queue_kind=0; initial_count=8; allocations=1; end
                    4: begin queue_kind=1; initial_count=8; allocations=1; end
                    5: begin queue_kind=0; tail=7; allocations=2; end
                    6: begin queue_kind=1; tail=7; allocations=2; end
                    7: begin queue_kind=0; phase_enables=8'h05; transaction=7; end
                    8: begin queue_kind=1; phase_enables=8'h05; transaction=8; end
                    9: begin queue_kind=0; generation=16'hfffe; allocations=1; end
                    10: begin queue_kind=1; tail=1; generation=16'hfffe; allocations=1; end
                    11: begin queue_kind=0; tail=2; generation=16'hffff; allocations=1; end
                    12: begin queue_kind=1; tail=3; generation=16'hffff; allocations=1; end
                    14,15,42,46: begin queue_kind=1; phase_enables=1; transaction=id; end
                    16,17,18,19,20,21,22,23,24,25,45: begin
                        configure_load_common(id);
                        case (id)
                            17,25: begin first_response_transaction=transaction+1; second_response_transaction=transaction+1; end
                            18: pending_rob=rob_idx+1;
                            19: pending_allocation=live_allocation+1;
                            20: pending_generation=generation+1;
                            21: owner_transaction=transaction+1;
                            22: flags=8'h57;
                            23: begin live_allocation=pending_allocation+1; sequence_mode=1; end
                            24: begin live_allocation=pending_allocation+1; sequence_mode=2; end
                            45: begin
                                transaction=id;
                                pending_transaction=transaction;
                                owner_transaction=transaction;
                                first_response_transaction=transaction;
                                second_response_transaction=transaction+1;
                                owner_rob=rob_idx+1;
                            end
                        endcase
                    end
                    26: begin store_address=64'h7000; load_address=64'h7000; store_mask=8'h0f; load_mask=8'hf0; end
                    27: begin store_address=64'h7000; load_address=64'h8000; store_mask=8'hff; load_mask=8'hff; end
                    30: begin tag=0; lq_slot=2; sq_slot=2; generation=7; transaction=30; queue_valids=1; lq_branch_mask=1; memory_mask=8'hff; end
                    31: begin tag=0; lq_slot=2; sq_slot=2; generation=7; transaction=31; queue_valids=2; sq_branch_mask=1; memory_mask=8'hff; end
                    32: begin tag=0; lq_slot=2; sq_slot=2; generation=7; transaction=32; queue_valids=1; lq_branch_mask=1; memory_mask=8'hff; mispredict=1; end
                    33: begin tag=0; lq_slot=2; sq_slot=2; generation=7; transaction=33; queue_valids=2; sq_branch_mask=1; memory_mask=8'hff; mispredict=1; end
                    34,35: begin tag=0; lq_slot=2; sq_slot=2; generation=7; transaction=id; queue_valids=5; lq_branch_mask=1; memory_mask=8'hff; mispredict=1; end
                    37: begin lq_slot=2; sq_slot=2; generation=7; transaction=37; mask=8'hff; end
                    41: begin queue_kind=0; phase_enables=8'h03; transaction=41; end
                    43: begin queue_kind=0; initial_count=8; phase_enables=8'h0c; transaction=43; end
                    44: begin queue_kind=1; initial_count=8; phase_enables=8'h0c; transaction=44; end
                    default: $fatal(1, "unsupported PATH_A case %0d", id);
                endcase
            end
            if ((id>=0 && id<=6) || (id>=9 && id<=12)) begin
                allocation_base = 32'h10000000 + id * 32'h100;
                address = 64'h1000 + id * 16;
            end else if ((id==7)||(id==8)||(id==14)||(id==15)||(id==41)||
                         (id==42)||(id==43)||(id==44)||(id==46)) begin
                allocation_base = 32'h20000000 + id * 32'h100;
                mask = 8'hff;
            end else if ((id>=30 && id<=35)) begin
                allocation_base = 32'h40000000 + id * 32'h100;
            end else if (id == 37) begin
                allocation_base = 32'h50000000 + id * 32'h100;
            end else if (id == 26 || id == 27) begin
                allocation_base = 32'h60000000 + id * 32'h100;
            end
        end
    endtask

    initial begin
        if (!$value$plusargs("CASE_ID=%d", case_id)) $fatal(1, "missing CASE_ID");
        if (!$value$plusargs("EXP0=%h", expected[0])) $fatal(1, "missing EXP0");
        if (!$value$plusargs("EXP1=%h", expected[1])) $fatal(1, "missing EXP1");
        if (!$value$plusargs("EXP2=%h", expected[2])) $fatal(1, "missing EXP2");
        if (!$value$plusargs("EXP3=%h", expected[3])) $fatal(1, "missing EXP3");
        if (!$value$plusargs("EXP4=%h", expected[4])) $fatal(1, "missing EXP4");
        if (!$value$plusargs("EXP5=%h", expected[5])) $fatal(1, "missing EXP5");
        if (!$value$plusargs("EXP6=%h", expected[6])) $fatal(1, "missing EXP6");
        if (!$value$plusargs("EXP7=%h", expected[7])) $fatal(1, "missing EXP7");
        configure_case(case_id);
        repeat (5) @(posedge ap_clk);
        ap_rst = 0;
        repeat (2) @(posedge ap_clk);
        @(negedge ap_clk); ap_start = 1;
        @(posedge ap_clk);
        @(negedge ap_clk); ap_start = 0;
        cycles = 0;
        while (!ap_done && cycles < 2000000) begin
            @(posedge ap_clk);
            cycles = cycles + 1;
        end
        if (!ap_done) $fatal(1, "G6_L1R80_PATH_A_TIMEOUT id=%0d", case_id);
        if (obs0 !== expected[0] || obs1 !== expected[1] ||
            obs2 !== expected[2] || obs3 !== expected[3] ||
            obs4 !== expected[4] || obs5 !== expected[5] ||
            obs6 !== expected[6] || obs7 !== expected[7])
            $fatal(1, "G6_L1R80_PATH_A_FAIL id=%0d observed=%h:%h:%h:%h:%h:%h:%h:%h expected=%h:%h:%h:%h:%h:%h:%h:%h",
                case_id, obs0,obs1,obs2,obs3,obs4,obs5,obs6,obs7,
                expected[0],expected[1],expected[2],expected[3],
                expected[4],expected[5],expected[6],expected[7]);
        $display("G6_L1R80_CASE_PASS id=%0d cycles=%0d", case_id, cycles);
        $finish;
    end
endmodule
