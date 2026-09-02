`timescale 1ns/1ps

module predictor_foundation_rtl_tb;
  reg ap_clk = 0;
  reg ap_rst = 1;
  reg ap_start = 0;
  wire ap_done;
  wire ap_idle;
  wire ap_ready;
  wire ap_local_block;
  wire ap_local_deadlock;

  reg runtime_reset;
  reg [31:0] active_generation;
  reg req_valid;
  reg [63:0] req_pc;
  reg [7:0] req_cfi_lane;
  reg [7:0] req_cfi_type;
  reg req_static_target_valid;
  reg [63:0] req_static_target;
  reg [31:0] req_generation;
  reg [63:0] req_token;
  reg resp_ready;
  reg update_valid;
  reg update_commit_qualified;
  reg [7:0] update_cfi_type;
  reg [63:0] update_pc;
  reg [15:0] update_metadata_token;
  reg update_taken;
  reg [31:0] update_generation;

  wire req_ready;
  wire req_ready_ap_vld;
  wire resp_valid;
  wire resp_valid_ap_vld;
  wire prediction_valid;
  wire prediction_valid_ap_vld;
  wire predicted_taken;
  wire predicted_taken_ap_vld;
  wire target_valid;
  wire target_valid_ap_vld;
  wire [63:0] target;
  wire target_ap_vld;
  wire [7:0] resp_cfi_lane;
  wire resp_cfi_lane_ap_vld;
  wire [7:0] resp_cfi_type;
  wire resp_cfi_type_ap_vld;
  wire [15:0] resp_metadata_token;
  wire resp_metadata_token_ap_vld;
  wire [31:0] resp_generation;
  wire resp_generation_ap_vld;
  wire [63:0] resp_request_token;
  wire resp_request_token_ap_vld;

  integer checks = 0;
  integer failures = 0;
  integer i;

  always #5 ap_clk = ~ap_clk;

  synth_predictor_foundation_top dut (
    .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
    .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start), .ap_done(ap_done),
    .ap_idle(ap_idle), .ap_ready(ap_ready), .reset(runtime_reset),
    .active_generation(active_generation), .req_valid(req_valid),
    .req_pc(req_pc), .req_cfi_lane(req_cfi_lane),
    .req_cfi_type(req_cfi_type),
    .req_static_target_valid(req_static_target_valid),
    .req_static_target(req_static_target), .req_generation(req_generation),
    .req_token(req_token), .resp_ready(resp_ready),
    .update_valid(update_valid),
    .update_commit_qualified(update_commit_qualified),
    .update_cfi_type(update_cfi_type), .update_pc(update_pc),
    .update_metadata_token(update_metadata_token), .update_taken(update_taken),
    .update_generation(update_generation), .req_ready(req_ready),
    .req_ready_ap_vld(req_ready_ap_vld), .resp_valid(resp_valid),
    .resp_valid_ap_vld(resp_valid_ap_vld),
    .prediction_valid(prediction_valid),
    .prediction_valid_ap_vld(prediction_valid_ap_vld),
    .predicted_taken(predicted_taken),
    .predicted_taken_ap_vld(predicted_taken_ap_vld),
    .target_valid(target_valid), .target_valid_ap_vld(target_valid_ap_vld),
    .target(target), .target_ap_vld(target_ap_vld),
    .resp_cfi_lane(resp_cfi_lane),
    .resp_cfi_lane_ap_vld(resp_cfi_lane_ap_vld),
    .resp_cfi_type(resp_cfi_type),
    .resp_cfi_type_ap_vld(resp_cfi_type_ap_vld),
    .resp_metadata_token(resp_metadata_token),
    .resp_metadata_token_ap_vld(resp_metadata_token_ap_vld),
    .resp_generation(resp_generation),
    .resp_generation_ap_vld(resp_generation_ap_vld),
    .resp_request_token(resp_request_token),
    .resp_request_token_ap_vld(resp_request_token_ap_vld)
  );

  task clear_inputs;
    begin
      runtime_reset = 0;
      active_generation = 32'd7;
      req_valid = 0;
      req_pc = 0;
      req_cfi_lane = 0;
      req_cfi_type = 0;
      req_static_target_valid = 0;
      req_static_target = 0;
      req_generation = 32'd7;
      req_token = 0;
      resp_ready = 0;
      update_valid = 0;
      update_commit_qualified = 0;
      update_cfi_type = 0;
      update_pc = 0;
      update_metadata_token = 0;
      update_taken = 0;
      update_generation = 32'd7;
    end
  endtask

  task step;
    begin
      @(negedge ap_clk);
      ap_start = 1;
      @(negedge ap_clk);
      while (!ap_done) @(negedge ap_clk);
      @(posedge ap_clk);
      #1 ap_start = 0;
      @(negedge ap_clk);
    end
  endtask

  task check_value;
    input condition;
    input [255:0] label;
    begin
      checks = checks + 1;
      if (!condition) begin
        failures = failures + 1;
        if (failures < 20) $display("FAIL,%0s", label);
      end
    end
  endtask

  task issue_request;
    input [63:0] pc;
    input [7:0] cfi_type;
    input [63:0] token_value;
    begin
      clear_inputs();
      req_valid = 1;
      req_pc = pc;
      req_cfi_type = cfi_type;
      req_cfi_lane = pc[1];
      req_static_target_valid = 1;
      req_static_target = pc + 64'h40;
      req_token = token_value;
      step();
      check_value(req_ready && !resp_valid, "request accepted without early response");
      clear_inputs();
      step();
      check_value(resp_valid, "response created after one logical cycle");
    end
  endtask

  task consume_response;
    begin
      clear_inputs();
      resp_ready = 1;
      step();
      check_value(resp_valid && !req_ready, "consume cycle remains blocking");
    end
  endtask

  task train_branch;
    input [63:0] pc;
    input taken_value;
    begin
      clear_inputs();
      update_valid = 1;
      update_commit_qualified = 1;
      update_cfi_type = 1;
      update_pc = pc;
      update_metadata_token = (pc >> 1) & 16'h00ff;
      update_taken = taken_value;
      step();
    end
  endtask

  initial begin
    clear_inputs();
    repeat (4) @(negedge ap_clk);
    ap_rst = 0;

    runtime_reset = 1;
    step();
    check_value(!req_ready && !resp_valid, "runtime reset clears protocol state");

    for (i = 0; i < 24; i = i + 1) begin
      issue_request(64'h1000 + i * 2, 8'd1, i);
      check_value(prediction_valid && !predicted_taken && !target_valid,
             "initial weak not taken");
      check_value(resp_metadata_token == (((64'h1000 + i * 2) >> 1) & 16'hff),
             "RV64C halfword index");
      consume_response();
    end

    issue_request(64'h2202, 8'd2, 64'h55);
    check_value(prediction_valid && predicted_taken && target_valid &&
           target == 64'h2242, "JAL static target");
    consume_response();

    issue_request(64'h2204, 8'd3, 64'h56);
    check_value(!prediction_valid && !predicted_taken && !target_valid,
           "JALR has no foundation prediction");
    consume_response();

    issue_request(64'h2206, 8'd0, 64'h57);
    check_value(!prediction_valid && !predicted_taken && !target_valid,
           "non-CFI has no prediction");
    consume_response();

    train_branch(64'h3000, 1'b1);
    issue_request(64'h3000, 8'd1, 64'h60);
    check_value(predicted_taken && target_valid && target == 64'h3040,
           "01 to 10 predicts taken");
    clear_inputs();
    req_valid = 1;
    req_pc = 64'h4000;
    req_cfi_type = 1;
    req_token = 64'hdead;
    step();
    check_value(resp_valid && !req_ready && resp_request_token == 64'h60,
           "held response stable and replacement blocked");
    consume_response();

    train_branch(64'h3000, 1'b1);
    train_branch(64'h3000, 1'b1);
    train_branch(64'h3000, 1'b0);
    issue_request(64'h3000, 8'd1, 64'h61);
    check_value(predicted_taken, "11 to 10 remains taken");
    consume_response();
    train_branch(64'h3000, 1'b0);
    issue_request(64'h3000, 8'd1, 64'h62);
    check_value(!predicted_taken, "10 to 01 becomes not taken");
    consume_response();
    train_branch(64'h3000, 1'b0);
    train_branch(64'h3000, 1'b0);
    issue_request(64'h3000, 8'd1, 64'h63);
    check_value(!predicted_taken, "00 saturates not taken");
    consume_response();

    clear_inputs();
    req_valid = 1;
    req_pc = 64'h5002;
    req_cfi_type = 1;
    req_static_target_valid = 1;
    req_static_target = 64'h5100;
    req_token = 64'h70;
    update_valid = 1;
    update_commit_qualified = 1;
    update_cfi_type = 1;
    update_pc = 64'h5002;
    update_metadata_token = 16'h01;
    update_taken = 1;
    step();
    clear_inputs();
    step();
    check_value(resp_valid && predicted_taken,
           "same-index update forwards new counter value");
    consume_response();

    issue_request(64'h6002, 8'd1, 64'h80);
    check_value(resp_metadata_token == 16'h01, "PC bit one contributes to index");
    consume_response();
    train_branch(64'h6002, 1'b1);
    issue_request(64'h6202, 8'd1, 64'h81);
    check_value(predicted_taken && resp_metadata_token == 16'h01,
           "different PCs alias at table wrap");
    consume_response();

    clear_inputs();
    runtime_reset = 1;
    active_generation = 32'd8;
    step();
    issue_request(64'h6002, 8'd1, 64'h82);
    check_value(!predicted_taken, "reset restores logical weak not taken");
    consume_response();

    $display("PREDICTOR_FOUNDATION_RTL,checks=%0d,failures=%0d", checks, failures);
    if (checks >= 50 && failures == 0) begin
      $display("GATE5_4_P2_PREDICTOR_RTL_PASS");
      $finish;
    end
    $fatal(1, "predictor RTL failures");
  end
endmodule
