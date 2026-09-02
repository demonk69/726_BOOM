`timescale 1ns/1ps

module predecode_rtl_tb;
  logic ap_start;
  logic [63:0] pc;
  logic [31:0] instruction;
  logic is_rvc;
  wire valid, is_cfi, is_conditional, is_jal, is_jalr, is_call, is_return;
  wire static_target_valid;
  wire [7:0] cfi_type, instruction_length;
  wire [63:0] static_target;
  wire valid_vld, is_cfi_vld, cfi_type_vld, cond_vld, jal_vld, jalr_vld;
  wire call_vld, return_vld, target_valid_vld, target_vld, length_vld;
  wire scalar_done, scalar_idle, scalar_ready, scalar_block, scalar_deadlock;

  logic [7:0] valid_mask;
  logic [63:0] lane0_pc, lane1_pc;
  logic [31:0] lane0_instruction, lane1_instruction;
  logic lane0_is_rvc, lane1_is_rvc;
  wire packet_has_cfi, selected_is_call, selected_is_return;
  wire selected_static_target_valid;
  wire [7:0] selected_cfi_lane, selected_cfi_type, younger_lane_mask;
  wire [7:0] predicted_taken_effective_mask;
  wire [63:0] selected_static_target;
  wire packet_done, packet_idle, packet_ready, packet_block, packet_deadlock;
  wire packet_has_cfi_vld, selected_lane_vld, selected_type_vld, selected_call_vld;
  wire selected_return_vld, selected_target_valid_vld, selected_target_vld;
  wire younger_vld, effective_vld;

  integer checks = 0;
  integer failures = 0;

  synth_predecode_top scalar_dut(
    .ap_local_block(scalar_block), .ap_local_deadlock(scalar_deadlock),
    .ap_start(ap_start), .ap_done(scalar_done), .ap_idle(scalar_idle),
    .ap_ready(scalar_ready), .pc(pc), .instruction(instruction), .is_rvc(is_rvc),
    .valid(valid), .valid_ap_vld(valid_vld), .is_cfi(is_cfi),
    .is_cfi_ap_vld(is_cfi_vld), .cfi_type(cfi_type),
    .cfi_type_ap_vld(cfi_type_vld), .is_conditional(is_conditional),
    .is_conditional_ap_vld(cond_vld), .is_jal(is_jal), .is_jal_ap_vld(jal_vld),
    .is_jalr(is_jalr), .is_jalr_ap_vld(jalr_vld), .is_call(is_call),
    .is_call_ap_vld(call_vld), .is_return(is_return),
    .is_return_ap_vld(return_vld), .static_target_valid(static_target_valid),
    .static_target_valid_ap_vld(target_valid_vld), .static_target(static_target),
    .static_target_ap_vld(target_vld), .instruction_length(instruction_length),
    .instruction_length_ap_vld(length_vld));

  synth_predecode_packet_top packet_dut(
    .ap_local_block(packet_block), .ap_local_deadlock(packet_deadlock),
    .ap_start(ap_start), .ap_done(packet_done), .ap_idle(packet_idle),
    .ap_ready(packet_ready), .valid_mask(valid_mask), .lane0_pc(lane0_pc),
    .lane0_instruction(lane0_instruction), .lane0_is_rvc(lane0_is_rvc),
    .lane1_pc(lane1_pc), .lane1_instruction(lane1_instruction),
    .lane1_is_rvc(lane1_is_rvc), .packet_has_cfi(packet_has_cfi),
    .packet_has_cfi_ap_vld(packet_has_cfi_vld),
    .selected_cfi_lane(selected_cfi_lane),
    .selected_cfi_lane_ap_vld(selected_lane_vld),
    .selected_cfi_type(selected_cfi_type),
    .selected_cfi_type_ap_vld(selected_type_vld),
    .selected_is_call(selected_is_call), .selected_is_call_ap_vld(selected_call_vld),
    .selected_is_return(selected_is_return),
    .selected_is_return_ap_vld(selected_return_vld),
    .selected_static_target_valid(selected_static_target_valid),
    .selected_static_target_valid_ap_vld(selected_target_valid_vld),
    .selected_static_target(selected_static_target),
    .selected_static_target_ap_vld(selected_target_vld),
    .younger_lane_mask(younger_lane_mask),
    .younger_lane_mask_ap_vld(younger_vld),
    .predicted_taken_effective_mask(predicted_taken_effective_mask),
    .predicted_taken_effective_mask_ap_vld(effective_vld));

  function automatic [31:0] enc_b(input integer signed imm, input [2:0] f3);
    reg [31:0] x;
    begin
      x = imm;
      enc_b = {x[12], x[10:5], 5'd2, 5'd1, f3, x[4:1], x[11], 7'h63};
    end
  endfunction

  function automatic [31:0] enc_j(input integer signed imm, input [4:0] rd);
    reg [31:0] x;
    begin
      x = imm;
      enc_j = {x[20], x[10:1], x[11], x[19:12], rd, 7'h6f};
    end
  endfunction

  function automatic [31:0] enc_jalr(input integer signed imm,
                                      input [4:0] rs1, input [4:0] rd);
    reg [31:0] x;
    begin
      x = imm;
      enc_jalr = {x[11:0], rs1, 3'b000, rd, 7'h67};
    end
  endfunction

  task automatic scalar_case(
      input [63:0] test_pc, input [31:0] test_inst, input test_rvc,
      input expected_cfi, input [7:0] expected_type,
      input expected_call, input expected_return,
      input expected_target_valid, input [63:0] expected_target);
    begin
      pc = test_pc; instruction = test_inst; is_rvc = test_rvc; #1;
      checks = checks + 1;
      if (!scalar_done || !valid_vld || !is_cfi_vld || !cfi_type_vld ||
          !target_vld || !length_vld || !valid || is_cfi !== expected_cfi ||
          cfi_type !== expected_type || is_call !== expected_call ||
          is_return !== expected_return ||
          static_target_valid !== expected_target_valid ||
          (expected_target_valid && static_target !== expected_target) ||
          instruction_length !== (test_rvc ? 8'd2 : 8'd4)) begin
        failures = failures + 1;
        $display("SCALAR_FAIL check=%0d inst=%08x type=%0d/%0d target=%h/%h",
                 checks, test_inst, cfi_type, expected_type, static_target, expected_target);
      end
    end
  endtask

  task automatic packet_case(
      input [7:0] mask, input [31:0] i0, input [31:0] i1,
      input expected_has, input [7:0] expected_lane,
      input [7:0] expected_type, input [7:0] expected_younger,
      input [7:0] expected_effective);
    begin
      valid_mask = mask; lane0_instruction = i0; lane1_instruction = i1; #1;
      checks = checks + 1;
      if (!packet_done || !packet_has_cfi_vld || !selected_lane_vld ||
          packet_has_cfi !== expected_has ||
          (expected_has && (selected_cfi_lane !== expected_lane ||
                            selected_cfi_type !== expected_type ||
                            younger_lane_mask !== expected_younger)) ||
          predicted_taken_effective_mask !== expected_effective) begin
        failures = failures + 1;
        $display("PACKET_FAIL check=%0d has=%0d/%0d lane=%0d/%0d younger=%0d/%0d",
                 checks, packet_has_cfi, expected_has, selected_cfi_lane,
                 expected_lane, younger_lane_mask, expected_younger);
      end
    end
  endtask

  initial begin
    ap_start = 1'b1;
    lane0_pc = 64'h1000; lane1_pc = 64'h1004;
    lane0_is_rvc = 0; lane1_is_rvc = 0;
    valid_mask = 0; lane0_instruction = 0; lane1_instruction = 0;

    scalar_case(64'h1000, enc_b(2, 0), 0, 1, 1, 0, 0, 1, 64'h1002);
    scalar_case(64'h1000, enc_b(-2, 1), 0, 1, 1, 0, 0, 1, 64'h0ffe);
    scalar_case(64'h1000, enc_b(4094, 4), 0, 1, 1, 0, 0, 1, 64'h1ffe);
    scalar_case(64'h1000, enc_b(-4096, 5), 0, 1, 1, 0, 0, 1, 64'h0);
    scalar_case(64'h1000, enc_b(126, 6), 0, 1, 1, 0, 0, 1, 64'h107e);
    scalar_case(64'h1000, enc_b(-254, 7), 0, 1, 1, 0, 0, 1, 64'h0f02);
    scalar_case(64'hffff_ffff_ffff_fffe, enc_b(2, 0), 0, 1, 1, 0, 0, 1, 64'h0);
    scalar_case(64'h2, enc_b(-2, 1), 0, 1, 1, 0, 0, 1, 64'h0);
    scalar_case(64'h2000, enc_b(0, 0), 0, 1, 1, 0, 0, 1, 64'h2000);
    scalar_case(64'h2000, enc_b(4, 0), 1, 1, 1, 0, 0, 1, 64'h2004); // C.BEQZ canonical
    scalar_case(64'h2002, enc_b(-4, 1), 1, 1, 1, 0, 0, 1, 64'h1ffe); // C.BNEZ canonical

    scalar_case(64'h4000, enc_j(2, 0), 0, 1, 2, 0, 0, 1, 64'h4002);
    scalar_case(64'h4000, enc_j(-2, 0), 0, 1, 2, 0, 0, 1, 64'h3ffe);
    scalar_case(64'h4000, enc_j(1048574, 1), 0, 1, 2, 1, 0, 1, 64'h103ffe);
    scalar_case(64'h4000, enc_j(-1048576, 5), 0, 1, 2, 1, 0, 1, 64'hffff_ffff_fff0_4000);
    scalar_case(64'h4000, enc_j(0, 7), 0, 1, 2, 0, 0, 1, 64'h4000);
    scalar_case(64'h4000, enc_j(2048, 31), 0, 1, 2, 0, 0, 1, 64'h4800);
    scalar_case(64'h5002, enc_j(-2048, 0), 1, 1, 2, 0, 0, 1, 64'h4802); // C.J canonical
    scalar_case(64'hffff_ffff_ffff_fffe, enc_j(2, 1), 0, 1, 2, 1, 0, 1, 64'h0);

    scalar_case(64'h6000, enc_jalr(0, 1, 0), 0, 1, 3, 0, 1, 0, 0);
    scalar_case(64'h6000, enc_jalr(0, 5, 0), 0, 1, 3, 0, 1, 0, 0);
    scalar_case(64'h6000, enc_jalr(0, 2, 1), 0, 1, 3, 1, 0, 0, 0);
    scalar_case(64'h6000, enc_jalr(-2048, 3, 5), 0, 1, 3, 1, 0, 0, 0);
    scalar_case(64'h6000, enc_jalr(2047, 7, 0), 0, 1, 3, 0, 0, 0, 0);
    scalar_case(64'h6002, enc_jalr(0, 1, 0), 1, 1, 3, 0, 1, 0, 0); // C.JR
    scalar_case(64'h6002, enc_jalr(0, 2, 1), 1, 1, 3, 1, 0, 0, 0); // C.JALR, link is PC+2 externally

    scalar_case(64'h7000, 32'h00100093, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h022081b3, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h0220c1b3, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h0000b103, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h0020b023, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h123452b7, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h12345297, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h00100073, 1, 0, 0, 0, 0, 0, 0); // C.EBREAK canonical
    scalar_case(64'h7000, 32'h0010d093, 1, 0, 0, 0, 0, 0, 0); // C.SRLI canonical
    scalar_case(64'h7000, 32'h00002063, 0, 0, 0, 0, 0, 0, 0); // reserved branch f3
    scalar_case(64'h7000, 32'h00003063, 0, 0, 0, 0, 0, 0, 0);
    scalar_case(64'h7000, 32'h00001067, 0, 0, 0, 0, 0, 0, 0); // reserved JALR f3
    scalar_case(64'h7000, 32'hffff_ffff, 0, 0, 0, 0, 0, 0, 0);

    packet_case(0, 32'h00100093, 32'h00200113, 0, 0, 0, 0, 0);
    packet_case(1, 32'h00100093, enc_b(2, 0), 0, 0, 0, 0, 1);
    packet_case(3, 32'h00100093, 32'h00200113, 0, 0, 0, 0, 3);
    packet_case(3, enc_b(2, 0), 32'h00200113, 1, 0, 1, 2, 1);
    packet_case(3, 32'h00100093, enc_b(2, 1), 1, 1, 1, 0, 3);
    packet_case(3, enc_b(2, 0), enc_b(-2, 1), 1, 0, 1, 2, 1);
    packet_case(3, enc_j(2, 0), enc_b(2, 0), 1, 0, 2, 2, 1);
    packet_case(3, enc_jalr(0, 2, 1), enc_j(2, 0), 1, 0, 3, 2, 1);
    lane0_is_rvc = 1; lane1_is_rvc = 0;
    packet_case(3, enc_b(2, 0), enc_j(2, 0), 1, 0, 1, 2, 1);
    lane0_is_rvc = 0; lane1_is_rvc = 1;
    packet_case(3, 32'h00100093, enc_j(-2, 0), 1, 1, 2, 0, 3);
    packet_case(1, enc_jalr(0, 1, 0), enc_j(2, 0), 1, 0, 3, 0, 1);
    packet_case(3, enc_j(2, 1), enc_jalr(0, 1, 0), 1, 0, 2, 2, 1);

    $display("PREDECODE_RTL,checks=%0d,failures=%0d", checks, failures);
    if (checks < 40 || failures != 0) $fatal(1, "GATE5_4_P1_PREDECODE_RTL_FAIL");
    $display("GATE5_4_P1_PREDECODE_RTL_PASS");
    $finish;
  end
endmodule
