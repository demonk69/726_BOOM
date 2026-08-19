`timescale 1ns/1ps

module fetch_packet_2lane_rtl_tb;
    reg ap_clk = 0;
    reg ap_rst = 1;
    reg ap_start = 0;
    reg [63:0] pc = 0;
    reg [31:0] instruction = 0;
    reg [31:0] fetch_id = 0;
    reg exception = 0;
    reg [63:0] exception_cause = 0;
    reg carry_valid = 0;
    reg [15:0] carry = 0;
    reg [63:0] carry_pc = 0;

    wire ap_done, ap_idle, ap_ready;
    wire packet_valid;
    wire [7:0] valid_mask;
    wire [63:0] lane0_pc, lane0_cause, lane1_pc, lane1_cause;
    wire [31:0] lane0_instruction, lane0_original, lane0_fetch_id;
    wire [31:0] lane1_instruction, lane1_original, lane1_fetch_id;
    wire lane0_is_rvc, lane0_exception, lane0_access_fault, lane0_misaligned;
    wire lane1_is_rvc, lane1_exception, lane1_access_fault, lane1_misaligned;
    wire [63:0] next_pc, next_carry_pc;
    wire next_carry_valid;
    wire [15:0] next_carry;
    reg mask_domain_ok;
    reg upper_mask_bits_ok;
    integer pass_count = 0;
    integer i;

    synth_fetch_packet_top dut (
        .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start),
        .ap_done(ap_done), .ap_idle(ap_idle), .ap_ready(ap_ready),
        .pc(pc), .instruction(instruction), .fetch_id(fetch_id),
        .exception(exception), .exception_cause(exception_cause),
        .carry_valid(carry_valid), .carry(carry), .carry_pc(carry_pc),
        .packet_valid(packet_valid), .valid_mask(valid_mask),
        .lane0_pc(lane0_pc), .lane0_instruction(lane0_instruction),
        .lane0_original(lane0_original), .lane0_fetch_id(lane0_fetch_id),
        .lane0_is_rvc(lane0_is_rvc), .lane0_exception(lane0_exception),
        .lane0_cause(lane0_cause), .lane0_access_fault(lane0_access_fault),
        .lane0_misaligned(lane0_misaligned),
        .lane1_pc(lane1_pc), .lane1_instruction(lane1_instruction),
        .lane1_original(lane1_original), .lane1_fetch_id(lane1_fetch_id),
        .lane1_is_rvc(lane1_is_rvc), .lane1_exception(lane1_exception),
        .lane1_cause(lane1_cause), .lane1_access_fault(lane1_access_fault),
        .lane1_misaligned(lane1_misaligned),
        .next_pc(next_pc), .next_carry_valid(next_carry_valid),
        .next_carry(next_carry), .next_carry_pc(next_carry_pc)
    );

    always #5 ap_clk = ~ap_clk;

    task fail(input [8*64-1:0] name, input [8*128-1:0] reason);
        begin
            $display("CASE_FAIL,%0s,%0s", name, reason);
            $fatal(1, "GATE5_3_B3I_FETCH_PACKET_RTL_FAIL case=%0s", name);
        end
    endtask

    task check_case(
        input [8*64-1:0] name,
        input [8*128-1:0] requirement,
        input condition
    );
        begin
            if (condition !== 1'b1) fail(name, requirement);
            pass_count = pass_count + 1;
            $display("CASE_PASS,%0s,%0s", name, requirement);
        end
    endtask

    task set_input(
        input [63:0] in_pc,
        input [31:0] in_instruction,
        input [31:0] in_fetch_id,
        input in_exception,
        input [63:0] in_cause,
        input in_carry_valid,
        input [15:0] in_carry,
        input [63:0] in_carry_pc
    );
        begin
            pc = in_pc;
            instruction = in_instruction;
            fetch_id = in_fetch_id;
            exception = in_exception;
            exception_cause = in_cause;
            carry_valid = in_carry_valid;
            carry = in_carry;
            carry_pc = in_carry_pc;
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
            if (ap_done !== 1'b1)
                fail("generated_control_timeout", "ap_done did not arrive");
            @(posedge ap_clk); #1;
        end
    endtask

    initial begin
        repeat (4) @(posedge ap_clk);
        @(negedge ap_clk); ap_rst = 0;

        // Two legal compressed parcels: C.NOP and C.ADDI x1,1.
        set_input(64'h1000, 32'h00850001, 32'h10203040, 0, 0, 0, 0, 0);
        invoke();
        check_case("cc_packet_mask3", "C+C produces valid mask3", packet_valid && valid_mask == 8'h03);
        check_case("cc_lane_pc_order", "C+C lane PCs are pc and pc+2", lane0_pc == 64'h1000 && lane1_pc == 64'h1002);
        check_case("cc_lane0_rvc_metadata", "lane0 preserves C.NOP original and RVC tag", lane0_original == 32'h0001 && lane0_is_rvc);
        check_case("cc_lane1_rvc_metadata", "lane1 preserves C.ADDI original and RVC tag", lane1_original == 32'h0085 && lane1_is_rvc);
        check_case("cc_lane_expansions", "both compressed lanes expose deterministic expansions", lane0_instruction == 32'h00000013 && lane1_instruction == 32'h00108093);
        check_case("cc_fetch_id_both_lanes", "fetch_id is copied to both complete lanes", lane0_fetch_id == 32'h10203040 && lane1_fetch_id == 32'h10203040);
        check_case("cc_no_faults", "legal compressed lanes have no exception subtype", !lane0_exception && !lane1_exception && !lane0_access_fault && !lane1_access_fault && !lane0_misaligned && !lane1_misaligned);
        check_case("cc_continuation", "C+C consumes the complete response", next_pc == 64'h1004 && !next_carry_valid);

        set_input(64'h2000, 32'h00108093, 32'h89abcdef, 0, 0, 0, 0, 0);
        invoke();
        check_case("ordinary32_mask1", "aligned ordinary32 produces only lane0", packet_valid && valid_mask == 8'h01);
        check_case("ordinary32_payload", "ordinary32 instruction is unchanged", lane0_instruction == 32'h00108093);
        check_case("ordinary32_metadata", "ordinary32 carries PC and fetch_id", lane0_pc == 64'h2000 && lane0_fetch_id == 32'h89abcdef);
        check_case("ordinary32_not_rvc", "ordinary32 has no RVC or fault metadata", !lane0_is_rvc && lane0_original == 0 && !lane0_exception);
        check_case("ordinary32_continuation", "ordinary32 advances four bytes without carry", next_pc == 64'h2004 && !next_carry_valid);
        check_case("ordinary32_no_partial_lane", "mask1 leaves lane1 deterministically unavailable", lane1_pc == 0 && lane1_instruction == 0 && lane1_fetch_id == 0 && !lane1_is_rvc && !lane1_exception);

        set_input(64'h3000, 32'h00930001, 32'h31, 0, 0, 0, 0, 0);
        invoke();
        check_case("c_partial_mask1", "C plus partial ordinary32 emits only complete C", packet_valid && valid_mask == 8'h01);
        check_case("c_partial_lane0_metadata", "complete lower C retains PC original and fetch_id", lane0_pc == 64'h3000 && lane0_original == 32'h0001 && lane0_fetch_id == 32'h31 && lane0_is_rvc);
        check_case("c_partial_carry_value", "upper ordinary32 parcel becomes carry", next_carry_valid && next_carry == 16'h0093);
        check_case("c_partial_carry_pc", "carry PC and next PC identify the partial parcel", next_carry_pc == 64'h3002 && next_pc == 64'h3002);
        check_case("c_partial_no_lane1", "partial instruction is never exposed as lane1", lane1_pc == 0 && lane1_instruction == 0 && lane1_original == 0 && lane1_fetch_id == 0);

        set_input(64'h4004, 32'h00850010, 32'h42, 0, 0, 1, 16'h0093, 64'h4002);
        invoke();
        check_case("carry_c_mask3", "completed carry plus upper C produces mask3", packet_valid && valid_mask == 8'h03);
        check_case("carry_c_lane0_assembly", "carry completion assembles exact ordinary32 bits", lane0_pc == 64'h4002 && lane0_instruction == 32'h00100093 && !lane0_is_rvc);
        check_case("carry_c_lane0_metadata", "carry completion receives response fetch_id", lane0_fetch_id == 32'h42 && lane0_original == 0 && !lane0_exception);
        check_case("carry_c_lane1_metadata", "upper C occupies lane1 with parcel metadata", lane1_pc == 64'h4006 && lane1_original == 32'h0085 && lane1_fetch_id == 32'h42 && lane1_is_rvc);
        check_case("carry_c_continuation", "carry plus C consumes upper parcel", next_pc == 64'h4008 && !next_carry_valid);

        set_input(64'h5004, 32'h00930010, 32'h52, 0, 0, 1, 16'h0093, 64'h5002);
        invoke();
        check_case("carry_new_carry_mask1", "carry plus new partial ordinary32 emits mask1", packet_valid && valid_mask == 8'h01);
        check_case("carry_new_carry_assembly", "old carry completes before new carry", lane0_pc == 64'h5002 && lane0_instruction == 32'h00100093);
        check_case("carry_new_carry_value", "upper parcel replaces consumed carry", next_carry_valid && next_carry == 16'h0093);
        check_case("carry_new_carry_pc", "replacement carry points at upper parcel", next_carry_pc == 64'h5006 && next_pc == 64'h5006);
        check_case("carry_new_carry_no_partial_lane", "new partial instruction is not a packet lane", lane1_pc == 0 && lane1_instruction == 0 && lane1_original == 0);

        set_input(64'h6002, 32'h00850000, 32'h62, 0, 0, 0, 0, 0);
        invoke();
        check_case("upper_start_c_mask1", "upper-half C start emits one complete lane", packet_valid && valid_mask == 8'h01);
        check_case("upper_start_c_metadata", "upper-half C uses requested PC and metadata", lane0_pc == 64'h6002 && lane0_original == 32'h0085 && lane0_fetch_id == 32'h62 && lane0_is_rvc);
        check_case("upper_start_c_continuation", "upper-half C advances by two", next_pc == 64'h6004 && !next_carry_valid);

        set_input(64'h6102, 32'h00930000, 32'h63, 0, 0, 0, 0, 0);
        invoke();
        check_case("upper_start_32_no_packet", "upper-half ordinary32 start has no complete lane", !packet_valid && valid_mask == 0);
        check_case("upper_start_32_carry", "upper-half ordinary32 start creates carry at requested PC", next_carry_valid && next_carry == 16'h0093 && next_carry_pc == 64'h6102 && next_pc == 64'h6102);
        check_case("upper_start_lanes_unavailable", "zero mask exposes neither lane", lane0_pc == 0 && lane1_pc == 0 && lane0_instruction == 0 && lane1_instruction == 0);

        set_input(64'h7000, 32'h00010000, 32'h71, 0, 0, 0, 0, 0);
        invoke();
        check_case("illegal_lower_mask1", "illegal lower C terminates with lane0 only", packet_valid && valid_mask == 1 && lane0_exception);
        check_case("illegal_lower_metadata", "illegal lower C preserves original PC fetch_id and cause", lane0_pc == 64'h7000 && lane0_original == 0 && lane0_fetch_id == 32'h71 && lane0_cause == 2 && lane0_is_rvc);
        check_case("illegal_lower_subtype", "illegal C is neither access fault nor misaligned", !lane0_access_fault && !lane0_misaligned);
        check_case("illegal_lower_no_partial_lane", "terminal lower fault exposes no upper lane or younger carry", lane1_pc == 0 && lane1_instruction == 0 && valid_mask[1] == 0 && !next_carry_valid);

        set_input(64'h7100, 32'h00000001, 32'h72, 0, 0, 0, 0, 0);
        invoke();
        check_case("illegal_upper_lane1", "illegal upper C is observable in lane1 and terminates carry", valid_mask == 3 && lane1_exception && lane1_pc == 64'h7102 && !next_carry_valid);
        check_case("illegal_upper_metadata", "illegal upper C preserves original fetch_id cause and subtype", lane1_original == 0 && lane1_fetch_id == 32'h72 && lane1_cause == 2 && lane1_is_rvc && !lane1_access_fault && !lane1_misaligned);

        set_input(64'h7200, 32'h0001001f, 32'h73, 0, 0, 0, 0, 0);
        invoke();
        check_case("long_lower_fault", "reserved long lower encoding is lane0 fault", valid_mask == 1 && lane0_exception && lane0_pc == 64'h7200);
        check_case("long_lower_metadata", "long lower fault preserves original fetch_id cause and subtype", lane0_original == 32'h001f && lane0_fetch_id == 32'h73 && lane0_cause == 2 && !lane0_is_rvc && !lane0_access_fault && !lane0_misaligned && !next_carry_valid);

        set_input(64'h7300, 32'h001f0001, 32'h74, 0, 0, 0, 0, 0);
        invoke();
        check_case("long_upper_fault", "reserved long upper encoding is terminal lane1 fault", valid_mask == 3 && lane1_exception && lane1_pc == 64'h7302 && !next_carry_valid);
        check_case("long_upper_metadata", "long upper fault preserves original fetch_id cause and subtype", lane1_original == 32'h001f && lane1_fetch_id == 32'h74 && lane1_cause == 2 && !lane1_is_rvc && !lane1_access_fault && !lane1_misaligned);

        set_input(64'h7404, 32'h00000010, 32'h75, 0, 0, 1, 16'h0093, 64'h7402);
        invoke();
        check_case("carry_illegal_upper_lane1", "carry completion precedes terminal illegal upper C fault", valid_mask == 3 && lane0_instruction == 32'h00100093 && lane1_exception && lane1_pc == 64'h7406 && lane1_is_rvc && lane1_cause == 2 && !next_carry_valid);

        set_input(64'h7504, 32'h001f0010, 32'h76, 0, 0, 1, 16'h0093, 64'h7502);
        invoke();
        check_case("carry_long_upper_lane1", "carry completion precedes terminal reserved long upper fault", valid_mask == 3 && lane0_instruction == 32'h00100093 && lane1_exception && lane1_pc == 64'h7506 && !lane1_is_rvc && lane1_original == 32'h001f && !next_carry_valid);

        set_input(64'h8000, 32'hdeadbeef, 32'h81, 1, 64'h000000000000000c, 0, 0, 0);
        invoke();
        check_case("access_fault_mask1", "requested lane access fault is the sole packet entry", packet_valid && valid_mask == 1 && lane0_exception);
        check_case("access_fault_metadata", "access fault preserves PC fetch_id and cause", lane0_pc == 64'h8000 && lane0_fetch_id == 32'h81 && lane0_cause == 64'hc);
        check_case("access_fault_subtype", "requested fault sets access subtype only", lane0_access_fault && !lane0_misaligned && !lane0_is_rvc && lane0_original == 0);
        check_case("access_fault_terminates", "requested fault consumes no parcel or carry", next_pc == 64'h8004 && !next_carry_valid && lane1_pc == 0 && valid_mask[1] == 0);

        set_input(64'h8104, 32'hcafef00d, 32'h82, 1, 64'hd, 1, 16'h0093, 64'h8102);
        invoke();
        check_case("carry_access_fault_attribution", "fault with carry is attributed to carry PC", valid_mask == 1 && lane0_pc == 64'h8102 && lane0_fetch_id == 32'h82 && lane0_cause == 64'hd && lane0_access_fault);
        check_case("carry_access_fault_termination", "fault with carry terminates and advances from carry PC", next_pc == 64'h8106 && !next_carry_valid && next_carry == 0 && next_carry_pc == 0);

        set_input(64'h8200, 0, 32'h83, 1, 0, 0, 0, 0);
        invoke();
        check_case("cause0_access_not_misaligned", "input exception cause0 remains access fault not misaligned", lane0_exception && lane0_cause == 0 && lane0_access_fault && !lane0_misaligned);

        // Sweep every constructor path used above and prove only masks 0/1/3 exist.
        mask_domain_ok = 1;
        upper_mask_bits_ok = 1;
        for (i = 0; i < 16; i = i + 1) begin
            case (i % 8)
                0: set_input(64'h9000 + i*8, 32'h00850001, i, 0, 0, 0, 0, 0);
                1: set_input(64'h9000 + i*8, 32'h00108093, i, 0, 0, 0, 0, 0);
                2: set_input(64'h9000 + i*8, 32'h00930001, i, 0, 0, 0, 0, 0);
                3: set_input(64'h9002 + i*8, 32'h00930000, i, 0, 0, 0, 0, 0);
                4: set_input(64'h9004 + i*8, 32'h00850010, i, 0, 0, 1, 16'h0093, 64'h9002 + i*8);
                5: set_input(64'h9004 + i*8, 32'h00930010, i, 0, 0, 1, 16'h0093, 64'h9002 + i*8);
                6: set_input(64'h9000 + i*8, 32'h00010000, i, 0, 0, 0, 0, 0);
                7: set_input(64'h9000 + i*8, 0, i, 1, i, 0, 0, 0);
            endcase
            invoke();
            mask_domain_ok = mask_domain_ok &&
                ((valid_mask == 0) || (valid_mask == 1) || (valid_mask == 3));
            upper_mask_bits_ok = upper_mask_bits_ok && (valid_mask[7:2] == 0);
        end
        check_case("mask_domain_never2", "all paths restrict masks to 0 1 or 3 never 2", mask_domain_ok);
        check_case("upper_mask_bits_unavailable", "all valid bits above the two helper lanes stay zero", upper_mask_bits_ok);

        set_input(64'hfffffffffffff000, 32'h00850001, 32'hfeed1234, 0, 0, 0, 0, 0);
        invoke();
        check_case("high_pc_metadata", "full-width PC and fetch_id survive C+C construction", lane0_pc == 64'hfffffffffffff000 && lane1_pc == 64'hfffffffffffff002 && lane0_fetch_id == 32'hfeed1234 && lane1_fetch_id == 32'hfeed1234);

        set_input(64'ha000, 32'h00850001, 32'ha5a55a5a, 0, 0, 0, 0, 0);
        invoke();
        check_case("helper_determinism_first", "first identical helper invocation has canonical outputs", valid_mask == 3 && lane0_instruction == 32'h13 && lane1_instruction == 32'h00108093 && next_pc == 64'ha004);
        invoke();
        check_case("helper_determinism_repeat", "repeated identical invocation has identical observable outputs", valid_mask == 3 && lane0_pc == 64'ha000 && lane1_pc == 64'ha002 && lane0_instruction == 32'h13 && lane1_instruction == 32'h00108093 && lane0_fetch_id == 32'ha5a55a5a && lane1_fetch_id == 32'ha5a55a5a && next_pc == 64'ha004 && !next_carry_valid);

        $display("GATE5_3_B3I_FETCH_PACKET_RTL_PASS cases=%0d", pass_count);
        $finish;
    end
endmodule
