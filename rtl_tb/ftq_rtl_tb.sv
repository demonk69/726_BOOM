`timescale 1ns/1ps

module ftq_rtl_tb;
  reg ap_clk = 0;
  reg ap_rst = 1;
  reg ap_start = 0;
  wire ap_done;
  wire ap_idle;
  wire ap_ready;
  wire ap_local_block;
  wire ap_local_deadlock;

  reg runtime_reset;
  reg alloc_valid;
  reg [63:0] alloc_pc;
  reg [7:0] alloc_mask;
  reg prediction_valid;
  reg predicted_taken;
  reg target_valid;
  reg [63:0] predicted_target;
  reg [7:0] cfi_lane;
  reg [7:0] cfi_type;
  reg [7:0] metadata_index;
  reg [31:0] predictor_generation;
  reg retire_valid;
  reg [7:0] retire_idx;
  reg [7:0] retire_lane;
  reg [31:0] retire_generation;
  reg squash_valid;
  reg [7:0] squash_idx;
  reg [7:0] squash_lane;
  reg [31:0] squash_generation;
  reg redirect_valid;
  reg [7:0] redirect_owner_idx;
  reg [31:0] redirect_owner_generation;
  reg [7:0] surviving_lane_mask;
  reg read_valid;
  reg [7:0] read_idx;
  reg [31:0] read_generation;

  wire alloc_ready;
  wire alloc_ready_ap_vld;
  wire alloc_accepted;
  wire alloc_accepted_ap_vld;
  wire alloc_invalid_mask;
  wire alloc_invalid_mask_ap_vld;
  wire [7:0] alloc_ftq_idx;
  wire alloc_ftq_idx_ap_vld;
  wire [31:0] alloc_generation;
  wire alloc_generation_ap_vld;
  wire retire_accepted;
  wire retire_accepted_ap_vld;
  wire retire_rejected;
  wire retire_rejected_ap_vld;
  wire squash_accepted;
  wire squash_accepted_ap_vld;
  wire squash_rejected;
  wire squash_rejected_ap_vld;
  wire redirect_accepted;
  wire redirect_accepted_ap_vld;
  wire redirect_rejected;
  wire redirect_rejected_ap_vld;
  wire reclaimed;
  wire reclaimed_ap_vld;
  wire [7:0] reclaimed_ftq_idx;
  wire reclaimed_ftq_idx_ap_vld;
  wire read_hit;
  wire read_hit_ap_vld;
  wire [63:0] read_pc;
  wire read_pc_ap_vld;
  wire [7:0] read_packet_mask;
  wire read_packet_mask_ap_vld;
  wire [7:0] read_live_mask;
  wire read_live_mask_ap_vld;
  wire read_prediction_valid;
  wire read_prediction_valid_ap_vld;
  wire read_predicted_taken;
  wire read_predicted_taken_ap_vld;
  wire read_target_valid;
  wire read_target_valid_ap_vld;
  wire [63:0] read_predicted_target;
  wire read_predicted_target_ap_vld;
  wire [7:0] read_cfi_lane;
  wire read_cfi_lane_ap_vld;
  wire [7:0] read_cfi_type;
  wire read_cfi_type_ap_vld;
  wire [7:0] read_metadata_index;
  wire read_metadata_index_ap_vld;
  wire [31:0] read_predictor_generation;
  wire read_predictor_generation_ap_vld;
  wire [31:0] read_entry_generation;
  wire read_entry_generation_ap_vld;
  wire empty;
  wire empty_ap_vld;
  wire full;
  wire full_ap_vld;
  wire [7:0] head;
  wire head_ap_vld;
  wire [7:0] tail;
  wire tail_ap_vld;
  wire [7:0] count;
  wire count_ap_vld;

  integer checks = 0;
  integer failures = 0;
  integer i;
  reg [7:0] saved_idx [0:31];
  reg [31:0] saved_gen [0:31];
  reg [7:0] first_idx;
  reg [31:0] first_gen;
  reg [7:0] second_idx;
  reg [31:0] second_gen;
  reg [7:0] third_idx;
  reg [31:0] third_gen;
  reg [7:0] new_idx;
  reg [31:0] new_gen;

  always #5 ap_clk = ~ap_clk;

  synth_ftq_32_top dut (
    .ap_local_block(ap_local_block), .ap_local_deadlock(ap_local_deadlock),
    .ap_clk(ap_clk), .ap_rst(ap_rst), .ap_start(ap_start), .ap_done(ap_done),
    .ap_idle(ap_idle), .ap_ready(ap_ready), .reset(runtime_reset),
    .alloc_valid(alloc_valid), .alloc_pc(alloc_pc), .alloc_mask(alloc_mask),
    .prediction_valid(prediction_valid), .predicted_taken(predicted_taken),
    .target_valid(target_valid), .predicted_target(predicted_target),
    .cfi_lane(cfi_lane), .cfi_type(cfi_type), .metadata_index(metadata_index),
    .predictor_generation(predictor_generation),
    .retire_valid(retire_valid), .retire_idx(retire_idx),
    .retire_lane(retire_lane), .retire_generation(retire_generation),
    .squash_valid(squash_valid), .squash_idx(squash_idx),
    .squash_lane(squash_lane), .squash_generation(squash_generation),
    .redirect_valid(redirect_valid), .redirect_owner_idx(redirect_owner_idx),
    .redirect_owner_generation(redirect_owner_generation),
    .surviving_lane_mask(surviving_lane_mask), .read_valid(read_valid),
    .read_idx(read_idx), .read_generation(read_generation),
    .alloc_ready(alloc_ready), .alloc_ready_ap_vld(alloc_ready_ap_vld),
    .alloc_accepted(alloc_accepted),
    .alloc_accepted_ap_vld(alloc_accepted_ap_vld),
    .alloc_invalid_mask(alloc_invalid_mask),
    .alloc_invalid_mask_ap_vld(alloc_invalid_mask_ap_vld),
    .alloc_ftq_idx(alloc_ftq_idx), .alloc_ftq_idx_ap_vld(alloc_ftq_idx_ap_vld),
    .alloc_generation(alloc_generation),
    .alloc_generation_ap_vld(alloc_generation_ap_vld),
    .retire_accepted(retire_accepted),
    .retire_accepted_ap_vld(retire_accepted_ap_vld),
    .retire_rejected(retire_rejected),
    .retire_rejected_ap_vld(retire_rejected_ap_vld),
    .squash_accepted(squash_accepted),
    .squash_accepted_ap_vld(squash_accepted_ap_vld),
    .squash_rejected(squash_rejected),
    .squash_rejected_ap_vld(squash_rejected_ap_vld),
    .redirect_accepted(redirect_accepted),
    .redirect_accepted_ap_vld(redirect_accepted_ap_vld),
    .redirect_rejected(redirect_rejected),
    .redirect_rejected_ap_vld(redirect_rejected_ap_vld),
    .reclaimed(reclaimed), .reclaimed_ap_vld(reclaimed_ap_vld),
    .reclaimed_ftq_idx(reclaimed_ftq_idx),
    .reclaimed_ftq_idx_ap_vld(reclaimed_ftq_idx_ap_vld),
    .read_hit(read_hit), .read_hit_ap_vld(read_hit_ap_vld),
    .read_pc(read_pc), .read_pc_ap_vld(read_pc_ap_vld),
    .read_packet_mask(read_packet_mask),
    .read_packet_mask_ap_vld(read_packet_mask_ap_vld),
    .read_live_mask(read_live_mask), .read_live_mask_ap_vld(read_live_mask_ap_vld),
    .read_prediction_valid(read_prediction_valid),
    .read_prediction_valid_ap_vld(read_prediction_valid_ap_vld),
    .read_predicted_taken(read_predicted_taken),
    .read_predicted_taken_ap_vld(read_predicted_taken_ap_vld),
    .read_target_valid(read_target_valid),
    .read_target_valid_ap_vld(read_target_valid_ap_vld),
    .read_predicted_target(read_predicted_target),
    .read_predicted_target_ap_vld(read_predicted_target_ap_vld),
    .read_cfi_lane(read_cfi_lane), .read_cfi_lane_ap_vld(read_cfi_lane_ap_vld),
    .read_cfi_type(read_cfi_type), .read_cfi_type_ap_vld(read_cfi_type_ap_vld),
    .read_metadata_index(read_metadata_index),
    .read_metadata_index_ap_vld(read_metadata_index_ap_vld),
    .read_predictor_generation(read_predictor_generation),
    .read_predictor_generation_ap_vld(read_predictor_generation_ap_vld),
    .read_entry_generation(read_entry_generation),
    .read_entry_generation_ap_vld(read_entry_generation_ap_vld),
    .empty(empty), .empty_ap_vld(empty_ap_vld), .full(full),
    .full_ap_vld(full_ap_vld), .head(head), .head_ap_vld(head_ap_vld),
    .tail(tail), .tail_ap_vld(tail_ap_vld), .count(count),
    .count_ap_vld(count_ap_vld)
  );

  task clear_inputs;
    begin
      runtime_reset = 0;
      alloc_valid = 0;
      alloc_pc = 0;
      alloc_mask = 0;
      prediction_valid = 0;
      predicted_taken = 0;
      target_valid = 0;
      predicted_target = 0;
      cfi_lane = 0;
      cfi_type = 0;
      metadata_index = 0;
      predictor_generation = 0;
      retire_valid = 0;
      retire_idx = 0;
      retire_lane = 0;
      retire_generation = 0;
      squash_valid = 0;
      squash_idx = 0;
      squash_lane = 0;
      squash_generation = 0;
      redirect_valid = 0;
      redirect_owner_idx = 0;
      redirect_owner_generation = 0;
      surviving_lane_mask = 0;
      read_valid = 0;
      read_idx = 0;
      read_generation = 0;
    end
  endtask

  // Keep ap_start asserted through ap_done, matching ap_ctrl_hs operation.
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
    input [511:0] label;
    begin
      checks = checks + 1;
      if (condition !== 1'b1) begin
        failures = failures + 1;
        if (failures < 20) $display("FAIL,%0s", label);
      end
    end
  endtask

  task reset_queue;
    begin
      clear_inputs();
      runtime_reset = 1;
      step();
    end
  endtask

  task allocate_packet;
    input [7:0] mask_value;
    input [63:0] pc_value;
    input pred_valid_value;
    input pred_taken_value;
    input target_valid_value;
    input [63:0] target_value;
    input [7:0] lane_value;
    input [7:0] type_value;
    input [7:0] metadata_value;
    begin
      clear_inputs();
      alloc_valid = 1;
      alloc_mask = mask_value;
      alloc_pc = pc_value;
      prediction_valid = pred_valid_value;
      predicted_taken = pred_taken_value;
      target_valid = target_valid_value;
      predicted_target = target_value;
      cfi_lane = lane_value;
      cfi_type = type_value;
      metadata_index = metadata_value;
      predictor_generation = {24'h0, metadata_value};
      step();
    end
  endtask

  task read_entry;
    input [7:0] index_value;
    input [31:0] generation_value;
    begin
      clear_inputs();
      read_valid = 1;
      read_idx = index_value;
      read_generation = generation_value;
      step();
    end
  endtask

  task retire_entry_lane;
    input [7:0] index_value;
    input [31:0] generation_value;
    input [7:0] lane_value;
    begin
      clear_inputs();
      retire_valid = 1;
      retire_idx = index_value;
      retire_generation = generation_value;
      retire_lane = lane_value;
      step();
    end
  endtask

  task squash_entry_lane;
    input [7:0] index_value;
    input [31:0] generation_value;
    input [7:0] lane_value;
    begin
      clear_inputs();
      squash_valid = 1;
      squash_idx = index_value;
      squash_generation = generation_value;
      squash_lane = lane_value;
      step();
    end
  endtask

  initial begin
    clear_inputs();
    repeat (4) @(negedge ap_clk);
    ap_rst = 0;

    reset_queue();
    check_value(empty && !full && count == 0, "runtime reset makes queue empty");
    check_value(head == 0 && tail == 0, "runtime reset restores pointers");
    check_value(!alloc_ready && !alloc_accepted && !reclaimed,
                "runtime reset suppresses normal operation");
    allocate_packet(8'b00, 64'h1000, 0, 0, 0, 0, 0, 0, 0);
    check_value(alloc_ready && !alloc_accepted, "mask 00 is a ready no-op");
    check_value(!alloc_invalid_mask && empty && count == 0,
                "mask 00 is legal and does not allocate");

    allocate_packet(8'b10, 64'h1008, 0, 0, 0, 0, 0, 0, 0);
    check_value(alloc_ready && !alloc_accepted, "mask 10 is not accepted");
    check_value(alloc_invalid_mask && empty && tail == 0,
                "mask 10 is reported illegal");

    allocate_packet(8'h07, 64'h1010, 0, 0, 0, 0, 0, 0, 0);
    check_value(alloc_invalid_mask && !alloc_accepted && count == 0,
                "mask high bits are illegal");

    // A predicted JAL packet exercises every stored field and the P2 BIM index.
    allocate_packet(8'b01, 64'h2000, 1, 1, 1, 64'h2800,
                    8'hff, 8'h06, 8'ha5);
    first_idx = alloc_ftq_idx;
    first_gen = alloc_generation;
    check_value(alloc_accepted && first_idx == 0, "mask 01 allocates lane zero");
    check_value(first_gen != 0 && count == 1 && !empty, "allocation has generation");
    read_entry(first_idx, first_gen);
    check_value(read_hit, "JAL metadata read hits");
    check_value(read_pc == 64'h2000, "JAL PC roundtrip");
    check_value(read_packet_mask == 8'b01 && read_live_mask == 8'b01,
                "JAL lane masks roundtrip");
    check_value(read_prediction_valid && read_predicted_taken,
                "JAL prediction roundtrip");
    check_value(read_target_valid && read_predicted_target == 64'h2800,
                "JAL target roundtrip");
    check_value(read_cfi_lane == 1 && read_cfi_type == 2,
                "JAL CFI fields are width-normalized");
    check_value(read_metadata_index == 8'ha5, "P2 BIM index roundtrip");
    check_value(read_predictor_generation == 32'ha5,
                "P2 predictor generation roundtrip");
    check_value(read_entry_generation == first_gen, "entry generation roundtrip");
    retire_entry_lane(first_idx, first_gen, 1);
    check_value(retire_rejected && !retire_accepted && !reclaimed,
                "absent lane retirement rejected");
    retire_entry_lane(first_idx, first_gen, 0);
    check_value(retire_accepted && reclaimed && reclaimed_ftq_idx == first_idx,
                "lane zero retires and reclaims mask 01");
    retire_entry_lane(first_idx, first_gen, 0);
    check_value(retire_rejected && !reclaimed, "duplicate retirement rejected");

    // JALR deliberately carries asserted raw prediction inputs while invalid.
    allocate_packet(8'b11, 64'h3000, 0, 1, 1, 64'hdeadbeef,
                    8'h02, 8'h07, 8'h3c);
    first_idx = alloc_ftq_idx;
    first_gen = alloc_generation;
    check_value(alloc_accepted && count == 1, "mask 11 allocates both lanes");
    read_entry(first_idx, first_gen);
    check_value(read_hit && read_packet_mask == 3 && read_live_mask == 3,
                "mask 11 read roundtrip");
    check_value(!read_prediction_valid && !read_predicted_taken,
                "JALR has no prediction");
    check_value(!read_target_valid && read_predicted_target == 0,
                "JALR has no predicted target");
    check_value(read_cfi_lane == 0 && read_cfi_type == 3,
                "JALR CFI fields are width-normalized");
    retire_entry_lane(first_idx, first_gen, 1);
    check_value(retire_accepted && !reclaimed && count == 1,
                "lane one retires independently");
    retire_entry_lane(first_idx, first_gen, 1);
    check_value(retire_rejected && !reclaimed, "duplicate lane one rejected");
    squash_entry_lane(first_idx, first_gen, 0);
    check_value(squash_accepted && reclaimed && empty,
                "lane zero squash completes packet");

    // A completed younger packet waits until all older packets leave the head.
    allocate_packet(1, 64'h4000, 0, 0, 0, 0, 0, 0, 8'h10);
    first_idx = alloc_ftq_idx;
    first_gen = alloc_generation;
    allocate_packet(1, 64'h4008, 0, 0, 0, 0, 0, 0, 8'h11);
    second_idx = alloc_ftq_idx;
    second_gen = alloc_generation;
    retire_entry_lane(second_idx, second_gen, 0);
    check_value(retire_accepted && !reclaimed && count == 2,
                "younger completion cannot reclaim out of order");
    read_entry(second_idx, second_gen);
    check_value(read_hit && read_live_mask == 0,
                "completed younger entry remains readable");
    retire_entry_lane(first_idx, first_gen, 0);
    check_value(retire_accepted && reclaimed && reclaimed_ftq_idx == first_idx,
                "older head reclaims first");
    clear_inputs();
    step();
    check_value(reclaimed && reclaimed_ftq_idx == second_idx && empty,
                "completed younger head reclaims next transaction");

    // Fill all 32 entries, then reclaim and allocate the same slot in one step.
    reset_queue();
    for (i = 0; i < 32; i = i + 1) begin
      allocate_packet(1, 64'h5000 + i * 8, 0, 0, 0, 0, 0, 0, i);
      saved_idx[i] = alloc_ftq_idx;
      saved_gen[i] = alloc_generation;
      check_value(alloc_accepted, "fill allocation accepted");
      check_value(alloc_ftq_idx == i, "fill allocation index ordered");
      check_value(count == i + 1, "fill count increments");
      check_value(head < 32 && tail < 32, "fill pointers remain in range");
    end
    check_value(full && !empty && count == 32, "depth 32 full asserted");
    allocate_packet(1, 64'h6000, 0, 0, 0, 0, 0, 0, 0);
    check_value(!alloc_ready && !alloc_accepted && full,
                "full queue backpressures allocation");
    retire_entry_lane(saved_idx[1], saved_gen[1], 0);
    check_value(retire_accepted && !reclaimed && full,
                "full queue preserves completed non-head");

    clear_inputs();
    retire_valid = 1;
    retire_idx = saved_idx[0];
    retire_generation = saved_gen[0];
    retire_lane = 0;
    alloc_valid = 1;
    alloc_mask = 1;
    alloc_pc = 64'h6100;
    metadata_index = 8'h61;
    step();
    new_gen = alloc_generation;
    check_value(retire_accepted && reclaimed && reclaimed_ftq_idx == saved_idx[0],
                "head retirement reclaims while full");
    check_value(alloc_ready && alloc_accepted && full && count == 32,
                "reclaim permits same-transaction replacement");
    check_value(alloc_ftq_idx == saved_idx[0] && new_gen != saved_gen[0],
                "tail wraps with a fresh generation");
    clear_inputs();
    step();
    check_value(reclaimed && reclaimed_ftq_idx == saved_idx[1] && count == 31,
                "ordered completed head reclaims after wrap");
    read_entry(saved_idx[0], saved_gen[0]);
    check_value(!read_hit, "wrapped slot rejects old generation");
    read_entry(saved_idx[0], new_gen);
    check_value(read_hit && read_pc == 64'h6100 && read_metadata_index == 8'h61,
                "wrapped slot accepts new generation");

    // Redirect retains its owner, intersects same-packet lanes, and kills younger entries.
    reset_queue();
    allocate_packet(3, 64'h7000, 0, 0, 0, 0, 0, 0, 8'h70);
    first_idx = alloc_ftq_idx;
    first_gen = alloc_generation;
    allocate_packet(3, 64'h7008, 1, 1, 1, 64'h7800, 1, 1, 8'h71);
    second_idx = alloc_ftq_idx;
    second_gen = alloc_generation;
    allocate_packet(3, 64'h7010, 0, 0, 0, 0, 0, 0, 8'h72);
    third_idx = alloc_ftq_idx;
    third_gen = alloc_generation;

    clear_inputs();
    redirect_valid = 1;
    redirect_owner_idx = second_idx;
    redirect_owner_generation = second_gen;
    surviving_lane_mask = 1;
    alloc_valid = 1;
    alloc_mask = 1;
    retire_valid = 1;
    retire_idx = second_idx;
    retire_generation = second_gen;
    retire_lane = 0;
    squash_valid = 1;
    squash_idx = first_idx;
    squash_generation = first_gen;
    squash_lane = 0;
    step();
    check_value(redirect_accepted && !redirect_rejected, "redirect owner accepted");
    check_value(retire_rejected && squash_rejected,
                "redirect rejects simultaneous lane events");
    check_value(!alloc_ready && !alloc_accepted, "redirect blocks allocation");
    check_value(count == 2 && tail == third_idx, "redirect truncates queue at owner");
    read_entry(second_idx, second_gen);
    check_value(read_hit && read_live_mask == 1 && read_packet_mask == 3,
                "redirect retains owner and kills same-packet lane one");
    check_value(read_pc == 64'h7008 && read_metadata_index == 8'h71,
                "redirect owner metadata retained");
    read_entry(third_idx, third_gen);
    check_value(!read_hit, "redirect kills younger entry");
    squash_entry_lane(second_idx, second_gen, 1);
    check_value(squash_rejected && !reclaimed,
                "redirect-killed owner lane rejects later event");
    retire_entry_lane(third_idx, third_gen, 0);
    check_value(retire_rejected, "killed younger generation rejects event");
    read_entry(second_idx, second_gen + 32'h10000);
    check_value(!read_hit, "stale generation read rejected");
    retire_entry_lane(second_idx, second_gen + 32'h10000, 0);
    check_value(retire_rejected && !reclaimed, "stale generation retire rejected");
    retire_entry_lane(first_idx, first_gen, 0);
    check_value(retire_accepted && !reclaimed, "redirect older lane zero retires");
    retire_entry_lane(first_idx, first_gen, 1);
    check_value(retire_accepted && reclaimed && reclaimed_ftq_idx == first_idx,
                "redirect older packet reclaims in order");
    retire_entry_lane(second_idx, second_gen, 0);
    check_value(retire_accepted && reclaimed && empty,
                "redirect owner eventually reclaims");

    clear_inputs();
    redirect_valid = 1;
    redirect_owner_idx = second_idx;
    redirect_owner_generation = second_gen;
    surviving_lane_mask = 3;
    step();
    check_value(redirect_rejected && !redirect_accepted,
                "stale redirect owner rejected");

    // Runtime reset must invalidate a live generation even when its slot is reused.
    allocate_packet(3, 64'h8000, 1, 1, 1, 64'h8800, 0, 1, 8'h80);
    first_idx = alloc_ftq_idx;
    first_gen = alloc_generation;
    reset_queue();
    check_value(empty && head == 0 && tail == 0 && count == 0,
                "runtime reset clears live queue");
    read_entry(first_idx, first_gen);
    check_value(!read_hit, "runtime reset invalidates stale read");
    retire_entry_lane(first_idx, first_gen, 0);
    check_value(retire_rejected && !reclaimed,
                "runtime reset invalidates stale retirement");
    allocate_packet(1, 64'h9000, 0, 0, 0, 0, 0, 0, 8'h90);
    new_idx = alloc_ftq_idx;
    new_gen = alloc_generation;
    check_value(alloc_accepted && new_idx == 0,
                 "post-reset allocation reuses slot zero");
    check_value(new_gen != first_gen && new_gen != 0,
                "runtime reset advances generation epoch");
    read_entry(first_idx, first_gen);
    check_value(!read_hit, "reused slot still rejects pre-reset generation");
    read_entry(new_idx, new_gen);
    check_value(read_hit && read_pc == 64'h9000 && read_entry_generation == new_gen,
                "post-reset generation reads successfully");

    $display("FTQ_RTL,checks=%0d,failures=%0d", checks, failures);
    if (checks >= 80 && failures == 0) begin
      $display("GATE5_4_F1_FTQ_RTL_PASS checks=%0d", checks);
      $finish;
    end
    $fatal(1, "FTQ RTL failures or insufficient checks");
  end
endmodule
