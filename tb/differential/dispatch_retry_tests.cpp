#include "boom_interfaces.hpp"
#include "reset.hpp"
#include <cstdio>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
void rename_module(BoomCoreState&);
void rob_allocate(BoomCoreState&);
void issue_module(BoomCoreState&);
void branch_module(BoomCoreState&);
}

static int passed = 0;
static int failed = 0;
#define TEST(name) std::printf("  [RETRY] %-56s ... ", name)
#define PASS() do { std::printf("PASS\n"); ++passed; } while (0)
#define CHECK(c, m) do { if (!(c)) { std::printf("FAIL: %s\n", m); ++failed; return; } } while (0)

static MicroOp make_uop(uint64_t token = 1) {
    MicroOp uop;
    uop.uopc = 50;
    uop.iq_type = IQ_ALU;
    uop.fu_code = FU_ALU;
    uop.debug_pc = RESET_VECTOR + token * 4;
    uop.debug_inst = (uint32_t)token;
    return uop;
}

static RenameDispatchPacket& seed_packet(BoomCoreState& state, uint64_t token, bool rob_owned) {
    RenameDispatchPacket& packet = state.rename.dispatch_packets[0];
    packet.valid = true;
    packet.rob_allocated = rob_owned;
    packet.uop = make_uop(token);
    return packet;
}

static void fill_iq(BoomCoreState& state) {
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; ++i) {
        IssueSlotEntry& slot = state.issue.alu_iq.entries[i];
        slot.valid = true;
        slot.request = false;
        slot.uop = make_uop(100 + i);
    }
    state.issue.alu_iq.count = ISSUE_QUEUE_ALU_DEPTH;
    state.issue.alu_iq.tail = 0;
}

static void t01_constructor_clear() {
    TEST("01 constructor clears persistent packet");
    BoomCoreState state;
    CHECK(!state.rename.dispatch_packets[0].valid, "packet starts valid");
    CHECK(!state.rename.dispatch_packets[0].rob_allocated, "packet starts ROB-owned");
    PASS();
}

static void t02_reset_clear() {
    TEST("02 reset clears valid, uop, and ROB ownership");
    BoomCoreState state;
    seed_packet(state, 2, true);
    ResetControllerState reset;
    boom_core_reset_step(state, reset);
    CHECK(!state.rename.dispatch_packets[0].valid, "reset retained valid");
    CHECK(!state.rename.dispatch_packets[0].rob_allocated, "reset retained ownership");
    CHECK(state.rename.dispatch_packets[0].uop.uopc == 0, "reset retained uop");
    PASS();
}

static void t03_rename_allocates_once() {
    TEST("03 retry does not repeat physical-register allocation");
    BoomCoreState state;
    MicroOp input = make_uop(3);
    input.rename.dst_rtype = DST_INT;
    input.rename.ldst = 1;
    state.decode.dec_valids[0] = true;
    state.decode.dec_uops[0] = input;
    uint8_t before = state.rename.int_free_list.count;
    boom::rename_module(state);
    uint8_t pdst = state.rename.dispatch_packets[0].uop.rename.pdst;
    CHECK(state.rename.dispatch_packets[0].valid, "rename did not create packet");
    CHECK(state.rename.int_free_list.count == before - 1, "first allocation count wrong");
    boom::rename_module(state);
    CHECK(state.rename.dispatch_packets[0].uop.rename.pdst == pdst, "pdst changed on retry");
    CHECK(state.rename.int_free_list.count == before - 1, "retry allocated another pdst");
    PASS();
}

static void t04_rob_full_retains() {
    TEST("04 ROB-full retry retains stable packet");
    BoomCoreState state;
    RenameDispatchPacket& packet = seed_packet(state, 4, false);
    state.rob.head = state.rob.tail = 0;
    state.rob.maybe_full = true;
    boom::rob_allocate(state);
    CHECK(packet.valid && !packet.rob_allocated, "ROB-full packet ownership changed");
    CHECK(packet.uop.debug_inst == 4, "ROB-full packet identity changed");
    PASS();
}

static void t05_rob_allocates_once() {
    TEST("05 repeated ROB stage allocates exactly once");
    BoomCoreState state;
    RenameDispatchPacket& packet = seed_packet(state, 5, false);
    boom::rob_allocate(state);
    uint8_t tail = state.rob.tail;
    uint8_t index = packet.uop.queue.rob_idx;
    uint32_t allocation_id = packet.uop.queue.rob_allocation_id;
    boom::rob_allocate(state);
    CHECK(packet.rob_allocated, "packet lost ROB ownership");
    CHECK(state.rob.tail == tail, "ROB tail advanced twice");
    CHECK(state.rob.entries[index].valid && state.rob.entries[index].uop.debug_inst == 5,
          "owned ROB entry changed");
    CHECK(allocation_id != 0 && packet.uop.queue.rob_allocation_id == allocation_id &&
          state.rob.entries[index].uop.queue.rob_allocation_id == allocation_id,
          "ROB allocation identity was not stable");
    PASS();
}

static void t06_direct_accept_consumes() {
    TEST("06 accepted direct bypass consumes packet");
    BoomCoreState state;
    seed_packet(state, 6, true);
    boom::issue_module(state);
    CHECK(!state.rename.dispatch_packets[0].valid, "accepted bypass retained packet");
    CHECK(state.issue.issued_valids[INT_ISSUE_LANE] &&
          state.issue.issued_uops[INT_ISSUE_LANE].debug_inst == 6,
          "accepted bypass did not issue packet");
    CHECK(state.issue.alu_iq.count == 0, "accepted bypass also inserted into IQ");
    PASS();
}

static void t07_iq_insert_consumes() {
    TEST("07 successful IQ insertion consumes packet");
    BoomCoreState state;
    IssueSlotEntry& old = state.issue.alu_iq.entries[0];
    old.valid = old.request = true;
    old.uop = make_uop(70);
    state.issue.alu_iq.count = state.issue.alu_iq.tail = 1;
    seed_packet(state, 7, true);
    boom::issue_module(state);
    CHECK(!state.rename.dispatch_packets[0].valid, "inserted packet remained valid");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.debug_inst == 7,
          "packet was not retained in IQ");
    PASS();
}

static void t08_full_iq_retries() {
    TEST("08 blocked full IQ retains ROB-owned packet");
    BoomCoreState state;
    fill_iq(state);
    state.issue.port_ready[MEM_ISSUE_LANE] = false;
    state.issue.port_ready[INT_ISSUE_LANE] = false;
    RenameDispatchPacket& packet = seed_packet(state, 8, true);
    boom::issue_module(state);
    CHECK(packet.valid && packet.rob_allocated, "blocked packet was consumed");
    CHECK(packet.uop.debug_inst == 8, "blocked packet identity changed");
    CHECK(state.issue.alu_iq.count == ISSUE_QUEUE_ALU_DEPTH, "blocked IQ changed size");
    PASS();
}

static void t09_exception_waits_for_rob() {
    TEST("09 exception waits until ROB owns it");
    BoomCoreState state;
    RenameDispatchPacket& packet = seed_packet(state, 9, false);
    packet.uop.exception = true;
    boom::issue_module(state);
    CHECK(packet.valid, "unowned exception was consumed");
    boom::rob_allocate(state);
    uint8_t index = packet.uop.queue.rob_idx;
    boom::issue_module(state);
    CHECK(!state.rename.dispatch_packets[0].valid, "ROB-owned exception was retained");
    CHECK(state.rob.entries[index].valid && state.rob.entries[index].exception,
          "exception was not preserved in ROB");
    CHECK(state.issue.alu_iq.count == 0 && !state.issue.issued_valids[0],
          "exception entered issue/execute path");
    PASS();
}

static void t10_frontend_decode_stall() {
    TEST("10 pending packet preserves decode while frontend decouples");
    BoomCoreState state;
    PipeSignals pipe;
    seed_packet(state, 10, false);
    state.frontend.reset_done = true;
    state.frontend.pc = RESET_VECTOR + 0x40;
    state.frontend.producer_valid = true;
    state.frontend.producer_uop.inst = 0x13;
    state.decode.dec_valids[0] = true;
    state.decode.dec_uops[0].debug_inst = 77;
    boom::frontend_module(state, pipe);
    boom::decode_module(state);
    CHECK(!state.frontend.stalled, "frontend stalled below fetch-buffer capacity");
    CHECK(state.frontend.fetch_buffer.count == 1, "producer was not buffered behind decode");
    CHECK(!pipe.imem_req.empty(), "decoupled frontend did not run ahead");
    CHECK(state.decode.dec_valids[0] && state.decode.dec_uops[0].debug_inst == 77,
           "stalled decode was overwritten");
    PASS();
}

static void t11_resolve_clears_mask() {
    TEST("11 correct branch resolve clears packet mask");
    BoomCoreState state;
    RenameDispatchPacket& packet = seed_packet(state, 11, false);
    packet.uop.branch.br_mask = 1;
    state.branch_state.active_mask = 1;
    state.branch_state.tag_valid[0] = true;
    ExecuteState::AluResult& result = state.execute.alu_results[0];
    result.valid = true;
    result.uop = make_uop(110);
    result.uop.branch.is_br = true;
    result.uop.branch.br_tag = 0;
    result.uop.queue.rob_idx = 0;
    state.rob.entries[0].valid = true;
    state.rob.entries[0].uop = result.uop;
    boom::branch_module(state);
    CHECK(packet.valid, "correct resolve killed packet");
    CHECK(packet.uop.branch.br_mask == 0, "resolved bit remained on packet");
    PASS();
}

static void t12_mispredict_kills() {
    TEST("12 mispredict kills matching pending packet");
    BoomCoreState state;
    RenameDispatchPacket& packet = seed_packet(state, 12, false);
    packet.uop.branch.br_mask = 1;
    state.branch_state.active_mask = 1;
    state.branch_state.tag_valid[0] = true;
    state.branch_state.snapshot_valid[0] = true;
    state.rob.entries[0].valid = true;
    ExecuteState::AluResult& result = state.execute.alu_results[0];
    result.valid = true;
    result.mispredict = true;
    result.redirect_pc = RESET_VECTOR + 0x80;
    result.uop = make_uop(120);
    result.uop.branch.is_br = true;
    result.uop.branch.br_tag = 0;
    result.uop.queue.rob_idx = 0;
    boom::branch_module(state);
    CHECK(!state.rename.dispatch_packets[0].valid, "wrong-path packet survived");
    CHECK(state.frontend.pc == RESET_VECTOR + 0x80, "redirect was not applied");
    PASS();
}

int main() {
    std::printf("=== BOOM-HLS W3 Stage 1 Dispatch Retry Tests ===\n");
    t01_constructor_clear();
    t02_reset_clear();
    t03_rename_allocates_once();
    t04_rob_full_retains();
    t05_rob_allocates_once();
    t06_direct_accept_consumes();
    t07_iq_insert_consumes();
    t08_full_iq_retries();
    t09_exception_waits_for_rob();
    t10_frontend_decode_stall();
    t11_resolve_clears_mask();
    t12_mispredict_kills();
    std::printf("Dispatch retry: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
