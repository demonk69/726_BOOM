`timescale 1ns/1ps

module g6_l1r_focused_tb;
    localparam F_INIT=0, F_OBSERVE=1, F_ALLOCATE=2, F_LSU_STEP=3;
    localparam F_RESPONSE=4, F_RECLAIM=5, F_OWNER=6, F_BRANCH=7;
    localparam F_COMMIT=8, F_RESET_BEGIN=9, F_RESET_STEP=10;
    localparam F_PIPE_FILL=11, F_PIPE_DRAIN=12, F_MUTATE=13, F_COMMIT_LSU=14;
    localparam M_SLOT_GEN=0, M_ROB_OWNER=1, M_ROB_FLAGS=2, M_PENDING=3;
    localparam M_QUEUE_OWNER=4, M_FLUSH=5, M_NEXT_TX=6, M_PRF=7, M_ROB_PTRS=8;

    reg ap_clk=0, ap_rst=1, ap_start=0;
    reg [7:0] opcode=0, kind=0, rob=0, slot=0, branch_mask=0, mask=0, size=0, flags=0, pdst=0;
    reg [15:0] generation=0;
    reg [31:0] allocation=0, transaction=0;
    reg [63:0] address=0, data=0;
    wire ap_done, ap_idle, ap_ready;
    wire [63:0] obs0, obs1, obs2, obs3, obs4, obs5;
    integer command_cycles, command_timeout, case_id, pass_count, failures, i, v;
    reg case_ok;
    reg [31:0] saved_tx;
    reg [15:0] saved_generation;

    g6_l1r_focused_top dut(
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .opcode(opcode), .kind(kind), .rob(rob), .slot(slot),
        .generation(generation), .branch_mask(branch_mask), .mask(mask),
        .size(size), .flags(flags), .pdst(pdst), .allocation(allocation),
        .transaction(transaction), .address(address), .data(data),
        .obs0(obs0), .obs1(obs1), .obs2(obs2), .obs3(obs3), .obs4(obs4), .obs5(obs5));

    always #5 ap_clk = ~ap_clk;

    task automatic command;
        input [7:0] c_op, c_kind, c_rob, c_slot;
        input [15:0] c_generation;
        input [7:0] c_branch, c_mask, c_size, c_flags, c_pdst;
        input [31:0] c_allocation, c_transaction;
        input [63:0] c_address, c_data;
        begin
            @(negedge ap_clk);
            opcode=c_op; kind=c_kind; rob=c_rob; slot=c_slot; generation=c_generation;
            branch_mask=c_branch; mask=c_mask; size=c_size; flags=c_flags; pdst=c_pdst;
            allocation=c_allocation; transaction=c_transaction; address=c_address; data=c_data;
            ap_start=1;
            @(posedge ap_clk);
            @(negedge ap_clk);
            ap_start=0;
            command_cycles=0;
            while (!ap_done && command_cycles < command_timeout) begin
                @(posedge ap_clk);
                command_cycles=command_cycles+1;
            end
            if (!ap_done) begin
                case_ok=0;
                $display("COMMAND_TIMEOUT case=%0d opcode=%0d", case_id, c_op);
            end
        end
    endtask

    task automatic simple;
        input [7:0] c_op, c_kind, c_rob, c_slot;
        input [15:0] c_generation;
        input [7:0] c_flags;
        input [31:0] c_allocation, c_transaction;
        input [63:0] c_address, c_data;
        begin
            command(c_op,c_kind,c_rob,c_slot,c_generation,0,8'hff,3,c_flags,5,
                    c_allocation,c_transaction,c_address,c_data);
        end
    endtask

    task automatic init_case;
        input integer id;
        begin
            case_id=id; case_ok=1; saved_tx=0; saved_generation=0;
            simple(F_INIT,0,0,0,0,0,0,0,0,0);
            if (obs0[23:8] !== 0 || obs0[2:1] !== 0) case_ok=0;
        end
    endtask

    task automatic allocate_entry;
        input is_store;
        input [7:0] c_rob, c_slot, c_branch, c_mask, c_size, c_flags, c_pdst;
        input [31:0] c_allocation;
        input [63:0] c_address, c_data;
        begin
            command(F_ALLOCATE,is_store,c_rob,c_slot,0,c_branch,c_mask,c_size,c_flags,c_pdst,
                    c_allocation,0,c_address,c_data);
        end
    endtask

    task automatic observe;
        input [7:0] c_rob, c_slot, c_pdst;
        begin
            command(F_OBSERVE,0,c_rob,c_slot,0,0,0,0,0,c_pdst,0,0,0,0);
        end
    endtask

    task automatic issue_and_drain;
        input [7:0] c_rob, c_slot;
        begin
            simple(F_LSU_STEP,0,c_rob,c_slot,0,0,0,0,0,0);
            if (!obs0[1] || !obs0[40] || !obs0[51]) case_ok=0;
            simple(F_PIPE_DRAIN,0,c_rob,c_slot,0,0,0,0,0,0);
            if (!obs0[41]) case_ok=0;
            saved_tx=obs5[31:0];
        end
    endtask

    task automatic finish_case;
        begin
            if (case_ok) begin
                pass_count=pass_count+1;
                $display("G6_L1R_CASE_PASS id=%0d",case_id);
            end else begin
                failures=failures+1;
                $display("G6_L1R_CASE_FAIL id=%0d obs0=%016h obs1=%016h obs2=%016h obs3=%016h",case_id,obs0,obs1,obs2,obs3);
            end
        end
    endtask

    task automatic fill_lq;
        input [31:0] base;
        begin
            for (i=0;i<8;i=i+1) begin
                allocate_entry(0,i,i,0,8'hff,3,0,0,base+i,64'h1000+i*8,0);
                if (!obs0[0]) case_ok=0;
            end
        end
    endtask

    task automatic fill_sq;
        input [31:0] base;
        begin
            for (i=0;i<8;i=i+1) begin
                allocate_entry(1,i,i,0,8'hff,3,0,0,base+i,64'h2000+i*8,base+i);
                if (!obs0[0]) case_ok=0;
            end
        end
    endtask

    task automatic run_base_case;
        input integer id;
        reg [31:0] a;
        begin
            init_case(id); a=32'h100000+id*32;
            case(id)
            0: if (obs0[23:8] != 0) case_ok=0;
            1: begin allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); if (obs0[19:8] != 12'h101) case_ok=0; end
            2: begin allocate_entry(1,2,0,0,8'hff,3,0,0,a,64'h2000,64'h55); if (obs0[23:12] != 12'h101) case_ok=0; end
            3: begin fill_lq(a); allocate_entry(0,31,0,0,8'hff,3,0,0,a+99,64'h3000,0); if (obs0[0] || obs0[19:8] != 12'h808) case_ok=0; end
            4: begin fill_sq(a); allocate_entry(1,31,0,0,8'hff,3,0,0,a+99,64'h3000,0); if (obs0[0] || obs0[23:12] != 12'h808) case_ok=0; end
            5: begin simple(F_MUTATE,M_SLOT_GEN,0,7,0,0,0,0,0,0); allocate_entry(0,1,7,0,8'hff,3,0,0,a,64'h1000,0); allocate_entry(0,2,0,0,8'hff,3,0,0,a+1,64'h1008,0); if (obs0[31:28]!=1 || !obs0[55]) case_ok=0; end
            6: begin simple(F_MUTATE,M_SLOT_GEN,0,7,0,1,0,0,0,0); allocate_entry(1,1,7,0,8'hff,3,0,0,a,64'h2000,1); allocate_entry(1,2,0,0,8'hff,3,0,0,a+1,64'h2008,2); if (obs0[39:36]!=1 || !obs0[56]) case_ok=0; end
            7: begin allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); issue_and_drain(1,0); simple(F_RESPONSE,0,1,0,0,0,0,saved_tx,0,64'h44); simple(F_MUTATE,M_SLOT_GEN,0,0,1,0,0,0,0,0); allocate_entry(0,2,0,0,8'hff,3,0,0,a+1,64'h1008,0); if (obs1[15:0]!=2) case_ok=0; end
            8: begin allocate_entry(1,1,0,0,8'hff,3,0,0,a,64'h2000,1); saved_generation=obs2[15:0]; simple(F_RECLAIM,0,1,0,saved_generation,0,a,0,0,0); simple(F_MUTATE,M_SLOT_GEN,0,0,saved_generation,1,0,0,0,0); allocate_entry(1,2,0,0,8'hff,3,0,0,a+1,64'h2008,2); if (obs2[15:0]!=2) case_ok=0; end
            9,10,11,12: begin simple(F_MUTATE,M_SLOT_GEN,0,id-9,(id<11)?16'hfffe:16'hffff,id[0],0,0,0,0); allocate_entry(id[0],3,id-9,0,8'hff,3,0,0,a,64'h3000,1); if ((id<11 && (id[0]?obs2[15:0]:obs1[15:0])!=16'hffff) || (id>=11 && (id[0]?obs2[15:0]:obs1[15:0])!=0)) case_ok=0; end
            13: begin allocate_entry(0,3,0,0,8'hff,3,0,0,a,64'h1000,0); saved_generation=obs1[15:0]; simple(F_MUTATE,M_QUEUE_OWNER,3,0,saved_generation+1,3,a,0,0,0); simple(F_LSU_STEP,0,3,0,0,0,0,0,0,0); if (obs0[1] || obs0[51]) case_ok=0; end
            14: begin allocate_entry(1,3,0,0,8'hff,3,0,0,a,64'h2000,1); saved_generation=obs2[15:0]; simple(F_OWNER,0,3,0,saved_generation+1,0,a,0,0,0); if (obs0[0]) case_ok=0; end
            15: begin allocate_entry(1,3,0,0,8'hff,3,0,0,a,64'h2000,1); saved_generation=obs2[15:0]; simple(F_RECLAIM,0,3,0,saved_generation+1,0,a,0,0,0); if (obs0[0] || obs0[23:12]!=12'h101) case_ok=0; end
            16: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); simple(F_RESPONSE,0,3,0,0,0,a,saved_tx,0,64'h1122); if (obs0[19:8]!=0 || !obs0[45] || obs4!=64'h1122) case_ok=0; end
            17: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); simple(F_RESPONSE,0,3,0,0,0,a,saved_tx+1,0,64'h1122); if (obs0[19:8]!=12'h101 || obs0[45]) case_ok=0; end
            18,19,20: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); simple(F_MUTATE,M_PENDING,(id==18)?4:3,0,(id==20)?obs1[15:0]+1:obs1[15:0],1,(id==19)?a+1:a,saved_tx,0,0); simple(F_RESPONSE,0,3,0,0,0,a,saved_tx,0,64'h99); if (obs0[19:8]!=12'h101 || obs0[45]) case_ok=0; end
            21: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); simple(F_MUTATE,M_QUEUE_OWNER,3,0,obs1[15:0],3,a,saved_tx+1,0,0); simple(F_RESPONSE,0,3,0,0,0,a,saved_tx,0,64'h99); if (obs0[45]) case_ok=0; end
            22: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); simple(F_MUTATE,M_QUEUE_OWNER,3,0,obs1[15:0],1,a,saved_tx,0,0); simple(F_RESPONSE,0,3,0,0,0,a,saved_tx,0,64'h99); if (obs0[45]) case_ok=0; end
            23: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); simple(F_MUTATE,M_FLUSH,0,0,0,1,0,0,0,0); simple(F_LSU_STEP,0,3,0,0,0,0,0,0,0); simple(F_MUTATE,M_FLUSH,0,0,0,0,0,0,0,0); simple(F_MUTATE,M_SLOT_GEN,0,0,obs1[15:0],0,0,0,0,0); allocate_entry(0,4,0,0,8'hff,3,0,5,a+1,64'h1008,0); simple(F_RESPONSE,0,4,0,0,0,a,saved_tx,0,64'h99); if (obs0[19:8]!=12'h101 || !obs0[55]) case_ok=0; end
            24,25: begin allocate_entry(0,3,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(3,0); if(id==24) simple(F_MUTATE,M_ROB_OWNER,3,0,0,0,a+1,0,0,0); else simple(F_MUTATE,M_PRF,0,0,0,0,0,0,0,64'haaaa); simple(F_RESPONSE,0,3,0,0,0,a,saved_tx+(id==25),0,64'h55); if (obs0[45] || obs0[46] || (id==25 && obs4!=64'haaaa)) case_ok=0; end
            26,27: begin allocate_entry(1,0,0,0,8'h0f,2,0,0,a,64'h9000,1); allocate_entry(0,1,1,0,8'hf0,2,0,0,a+1,(id==26)?64'h9000:64'ha000,0); simple(F_LSU_STEP,0,1,1,0,0,0,0,0,0); if (obs0[1] || obs0[51]) case_ok=0; end
            28: begin simple(F_PIPE_FILL,0,0,0,0,0,0,32'hdead,64'hdead0000,0); allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); simple(F_LSU_STEP,0,1,0,0,0,0,0,0,0); if (obs0[1] || obs0[51] || !obs0[40]) case_ok=0; simple(F_PIPE_DRAIN,0,1,0,0,0,0,0,0,0); issue_and_drain(1,0); end
            29: begin simple(F_PIPE_FILL,0,0,0,0,0,0,32'hdead,64'hdead0000,0); allocate_entry(1,0,0,0,8'hff,3,0,0,a,64'h2000,1); simple(F_MUTATE,M_ROB_FLAGS,0,0,0,1,a,0,0,0); simple(F_MUTATE,M_ROB_PTRS,0,1,0,0,0,0,0,0); simple(F_COMMIT,0,0,0,0,0,0,0,0,0); if (obs0[51] || obs0[23:12]!=12'h101) case_ok=0; simple(F_PIPE_DRAIN,0,0,0,0,0,0,0,0,0); simple(F_COMMIT,0,0,0,0,0,0,0,0,0); if (!obs0[40] || obs0[23:12]!=0) case_ok=0; simple(F_PIPE_DRAIN,0,0,0,0,0,0,0,0,0); if (!obs0[42] || !obs0[43]) case_ok=0; end
            30,31,32,33: begin allocate_entry(id[0],1,0,4,8'hff,3,0,0,a,64'h1000,1); simple(F_BRANCH,0,0,2,0,(id>=32),a+1,0,64'h4000,0); if (id<32 && (id[0]?(!obs0[56]||obs2[63:56]!=0):(!obs0[55]||obs1[63:56]!=0))) case_ok=0; if (id>=32 && (obs0[23:8]!=0)) case_ok=0; end
            34,35: begin allocate_entry(0,1,0,4,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(1,0); simple(F_BRANCH,0,0,2,0,1,a+1,0,64'h4000,0); if(obs0[1]||obs0[19:8]!=0) case_ok=0; if(id==35) begin simple(F_RESPONSE,0,1,0,0,0,a,saved_tx,0,64'h88); if(obs0[45]||obs0[46]) case_ok=0; end end
            36: begin allocate_entry(0,1,0,0,8'hff,3,0,5,a,64'h1000,0); issue_and_drain(1,0); allocate_entry(1,2,1,0,8'hff,3,0,0,a+1,64'h2000,1); simple(F_BRANCH,0,0,2,0,0,a+2,0,0,0); simple(F_MUTATE,M_ROB_FLAGS,0,0,0,5,a+2,0,5,0); simple(F_MUTATE,M_ROB_PTRS,0,1,0,0,0,0,0,0); simple(F_COMMIT,0,0,0,0,0,0,0,0,0); if(!obs0[2]||!obs0[48]) case_ok=0; simple(F_LSU_STEP,0,1,0,0,0,0,0,0,0); simple(F_RESPONSE,0,1,0,0,0,a,saved_tx,0,64'h99); if(obs0[45]||obs0[23:8]!=0) case_ok=0; end
            37: begin allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); allocate_entry(1,2,1,0,8'hff,3,0,0,a+1,64'h2000,1); simple(F_MUTATE,M_FLUSH,0,0,0,1,0,0,0,0); simple(F_LSU_STEP,0,1,0,0,0,0,0,0,0); if(obs0[23:8]!=0) case_ok=0; end
            38,39,40: begin if(id>=39) allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); if(id==40) fill_sq(a+10); simple(F_RESET_BEGIN,0,0,0,0,0,0,0,0,0); for(i=0;i<400 && !obs0[3];i=i+1) simple(F_RESET_STEP,0,0,0,0,0,0,0,0,0); if(!obs0[3]||obs0[23:8]!=0) case_ok=0; end
            41: begin allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); allocate_entry(0,2,1,0,8'hff,3,0,0,a+1,64'h1008,0); issue_and_drain(1,0); simple(F_RESPONSE,0,1,0,0,0,a,saved_tx,0,1); if(obs0[19:8]!=12'h101) case_ok=0; end
            42: begin allocate_entry(1,1,0,0,8'hff,3,0,0,a,64'h2000,1); saved_generation=obs2[15:0]; simple(F_RECLAIM,0,1,0,saved_generation,0,a,0,0,0); if(obs0[23:12]!=0) case_ok=0; end
            43: begin fill_lq(a); issue_and_drain(0,0); simple(F_RESPONSE,0,0,0,0,0,a,saved_tx,0,1); allocate_entry(0,16,0,0,8'hff,3,0,0,a+99,64'h3000,0); if(obs0[19:8]!=12'h808) case_ok=0; end
            44: begin fill_sq(a); observe(0,0,0); saved_generation=obs2[15:0]; simple(F_RECLAIM,0,0,0,saved_generation,0,a,0,0,0); allocate_entry(1,16,0,0,8'hff,3,0,0,a+99,64'h3000,1); if(obs0[23:12]!=12'h808) case_ok=0; end
            45: begin allocate_entry(0,5,0,0,8'hff,3,0,0,a,64'h1000,0); issue_and_drain(5,0); simple(F_RESPONSE,0,6,0,0,0,a,saved_tx,0,1); if(obs0[19:8]!=12'h101) case_ok=0; end
            46: begin allocate_entry(1,5,0,0,8'hff,3,0,0,a,64'h2000,1); saved_generation=obs2[15:0]; simple(F_OWNER,0,5,0,saved_generation,0,a,0,0,0); if(!obs0[0]) case_ok=0; simple(F_OWNER,0,6,0,saved_generation,0,a,0,0,0); if(obs0[0]) case_ok=0; end
            47: begin allocate_entry(0,5,0,0,8'hff,3,0,0,a,64'h1000,0); simple(F_MUTATE,M_ROB_OWNER,5,0,0,0,a+1,0,0,0); simple(F_LSU_STEP,0,5,0,0,0,0,0,0,0); if(obs0[1]||obs0[51]) case_ok=0; end
            48: begin allocate_entry(1,0,0,0,8'hff,3,0,0,a,64'h2000,1); allocate_entry(0,1,1,0,8'hff,3,0,0,a+1,64'h1000,0); simple(F_MUTATE,M_ROB_FLAGS,0,0,0,1,a,0,0,0); simple(F_MUTATE,M_ROB_PTRS,0,2,0,0,0,0,0,0); simple(F_COMMIT_LSU,0,1,1,0,0,0,0,0,0); if(!obs0[40]||obs0[1]) case_ok=0; end
            49: begin allocate_entry(0,1,0,0,8'hff,3,0,0,a,64'h1000,0); issue_and_drain(1,0); simple(F_RESET_BEGIN,0,0,0,0,0,0,0,0,0); for(i=0;i<400 && !obs0[3];i=i+1) simple(F_RESET_STEP,0,0,0,0,0,0,0,0,0); if(obs0[1]||obs0[23:8]!=0) case_ok=0; end
            endcase
            finish_case();
        end
    endtask

    task automatic run_variant_case;
        input integer id;
        reg [7:0] vs, vm;
        reg [15:0] vg;
        reg [31:0] va, vt;
        begin
            init_case(id); v=id-50; vs=v[2:0];
            case(v%6) 0:vg=0; 1:vg=1; 2:vg=16'h7fff; 3:vg=16'hfffd; 4:vg=16'hfffe; default:vg=16'hffff; endcase
            vm=(v<8)?(1<<v):(v<16?(8'hff>>(v&7)):(8'ha5^v));
            va=32'h80000000|(v<<12)|vm; vt=1+v*32'h10101;
            simple(F_MUTATE,M_SLOT_GEN,0,vs,vg,v[0],0,0,0,0);
            simple(F_MUTATE,M_NEXT_TX,0,vs,0,0,0,vt,0,0);
            if(v%5==0) begin allocate_entry(0,(v*7+3)&31,vs,0,vm,v&3,v[0],5,va,64'h4000+v*8,0); issue_and_drain((v*7+3)&31,vs); if(saved_tx!=vt) case_ok=0; simple(F_RESPONSE,0,(v*7+3)&31,vs,0,0,va,saved_tx,0,64'h100+v); if(obs0[45]==0) case_ok=0; end
            else if(v%5==1) begin allocate_entry(1,(v*7+3)&31,vs,0,vm,v&3,0,0,va,64'h5000+v*8,v); saved_generation=obs2[15:0]; simple(F_OWNER,0,(v*7+3)&31,vs,saved_generation,0,va,0,0,0); if(!obs0[0]) case_ok=0; simple(F_RECLAIM,0,(v*7+3)&31,vs,saved_generation,0,va,0,0,0); end
            else if(v%5==2) begin allocate_entry(0,(v*7+3)&31,vs,0,vm,v&3,0,5,va,64'h6000+v*8,0); issue_and_drain((v*7+3)&31,vs); simple(F_RESPONSE,0,(v*7+3)&31,vs,0,0,va,saved_tx+1,0,v); if(obs0[45]) case_ok=0; simple(F_RESPONSE,0,(v*7+3)&31,vs,0,0,va,saved_tx,0,v); if(!obs0[45]) case_ok=0; end
            else if(v%5==3) begin allocate_entry(0,(v*7+3)&31,vs,0,vm,v&3,0,0,va,64'h7000+v*8,0); simple(F_MUTATE,M_FLUSH,0,vs,0,1,0,0,0,0); simple(F_LSU_STEP,0,0,vs,0,0,0,0,0,0); if(obs0[23:8]!=0) case_ok=0; end
            else begin allocate_entry(0,(v*7+3)&31,vs,1<<(v&7),vm,v&3,0,0,va,64'h8000+v*8,0); simple(F_BRANCH,0,0,v&7,0,1,va-1,0,64'h9000+v*4,0); if(obs0[19:8]!=0) case_ok=0; end
            finish_case();
        end
    endtask

    initial begin
        command_timeout=2000000;
        if (!$value$plusargs("COMMAND_TIMEOUT=%d",command_timeout)) command_timeout=2000000;
        pass_count=0; failures=0; case_ok=1;
        repeat(5) @(posedge ap_clk); ap_rst=0; repeat(2) @(posedge ap_clk);
        for(case_id=0;case_id<50;case_id=case_id+1) run_base_case(case_id);
        for(case_id=50;case_id<80;case_id=case_id+1) run_variant_case(case_id);
        if(pass_count!=80 || failures!=0) $fatal(1,"G6_L1R_FOCUSED_RTL_FAIL pass=%0d fail=%0d",pass_count,failures);
        $display("G6_L1R_FOCUSED_RTL_PASS cases=80");
        $finish;
    end
endmodule
