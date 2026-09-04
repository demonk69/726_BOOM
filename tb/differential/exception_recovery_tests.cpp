#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include <cstdint>
#include <iostream>

namespace boom {
void rob_commit_module(BoomCoreState&, PipeSignals&);
void frontend_module(BoomCoreState&, PipeSignals&);
}

static int failures = 0;
static uint64_t checks = 0;

#define CHECK(c, m) do { checks++; if (!(c)) { \
    std::cerr << "FAIL: " << m << " line=" << __LINE__ << "\n"; failures++; \
} } while (0)

static void init_running(BoomCoreState& s) {
    s.rob.state = ROB_NORMAL;
    s.frontend.reset_done = true;
    s.frontend.epoch = 11;
    s.csr.priv = PRV_M;
    s.csr.mstatus = 1ULL << 3;
    for (int i = 0; i < LOGICAL_REG_COUNT; i++) {
        s.rename.int_map_table.committed_map_table[i] = (uint8_t)i;
        s.rename.int_map_table.map_table[i] = (uint8_t)i;
    }
}

static void seed_exception(BoomCoreState& s, uint8_t head, uint64_t pc,
                           uint64_t cause, uint32_t inst, bool rvc) {
    s.rob.head = head;
    s.rob.tail = (uint8_t)((head + 2) % ROB_DEPTH);
    RobEntry& owner = s.rob.entries[head];
    owner.valid = true;
    owner.busy = false;
    owner.exception = true;
    owner.uop.uopc = 255;
    owner.uop.inst = inst;
    owner.uop.debug_inst = inst;
    owner.uop.is_rvc = rvc;
    owner.uop.debug_pc = pc;
    owner.uop.exception = true;
    owner.uop.exc_cause = cause;
    owner.uop.queue.rob_idx = head;
    owner.uop.queue.rob_allocation_id = 101;

    RobEntry& younger = s.rob.entries[(head + 1) % ROB_DEPTH];
    younger.valid = true;
    younger.busy = true;
    younger.uop.uopc = 1;
    younger.uop.queue.rob_idx = (uint8_t)((head + 1) % ROB_DEPTH);
    younger.uop.queue.rob_allocation_id = 102;
    younger.uop.rename.pdst = 40;
}

static void check_take(uint64_t pc, uint64_t cause, uint32_t inst, bool rvc,
                       uint64_t expected_tval, uint8_t head) {
    BoomCoreState s;
    PipeSignals p;
    init_running(s);
    seed_exception(s, head, pc, cause, inst, rvc);
    s.rob.next_allocation_id = 777;
    s.lsu.next_transaction_id = 888;
    s.rename.int_map_table.map_table[5] = 40;
    s.rename.int_free_list.busy_table[40] = true;
    s.decode.dec_valids[0] = true;
    s.rename.dispatch_packets[0].valid = true;
    s.issue.alu_iq.entries[0].valid = true;
    s.issue.alu_iq.count = 1;
    s.execute.divider.token_valid = true;
    s.completion.int_execute.valid = true;
    s.lsu.ldq[0].valid = true;
    s.lsu.ldq_count = 1;
    s.branch_state.active_mask = 1;

    boom::rob_commit_module(s, p);
    CHECK(s.exception_commit.valid, "exception event missing");
    CHECK(s.exception_commit.pc == pc, "EPC event mismatch");
    CHECK(s.exception_commit.cause == cause, "cause event mismatch");
    CHECK(s.exception_commit.tval == expected_tval, "tval mismatch");
    CHECK(s.exception_commit.target == (BOOM_TRAP_VECTOR & ~3ULL), "target mismatch");
    CHECK(s.csr.mepc == pc, "mepc mismatch");
    CHECK(s.csr.mcause == cause, "mcause mismatch");
    CHECK(s.csr.mtval == expected_tval, "mtval mismatch");
    CHECK(s.csr.priv == PRV_M, "privilege mismatch");
    CHECK(((s.csr.mstatus >> 3) & 1) == 0, "MIE not cleared");
    CHECK(((s.csr.mstatus >> 7) & 1) == 1, "MPIE not captured");
    CHECK(s.frontend_redirect.valid, "redirect missing");
    CHECK(s.frontend_redirect.cause == FRONTEND_REDIRECT_EXCEPTION, "redirect cause");
    CHECK(!s.io_trap, "recoverable exception asserted terminal io_trap");
    CHECK(s.global_flush, "recovery flush missing");
    CHECK(s.rob.head == 0 && s.rob.tail == 0 && !s.rob.maybe_full, "ROB not empty");
    CHECK(s.rob.state == ROB_NORMAL, "ROB terminal after take");
    CHECK(s.rob.next_allocation_id == 777, "allocation identity rewound");
    CHECK(s.lsu.next_transaction_id == 888, "memory identity rewound");
    CHECK(!s.decode.dec_valids[0], "decode survived");
    CHECK(!s.rename.dispatch_packets[0].valid, "rename dispatch survived");
    CHECK(s.issue.alu_iq.count == 0, "issue queue survived");
    CHECK(!s.execute.divider.token_valid, "divider survived");
    CHECK(!s.completion.int_execute.valid, "completion survived");
    CHECK(s.lsu.ldq_count == 0, "LDQ survived");
    CHECK(s.branch_state.active_mask == 0, "branch state survived");
    CHECK(s.rename.int_map_table.map_table[5] == 5, "committed map not restored");
    CHECK(!p.commit_trace.empty(), "exception trace missing");
    CommitEntry ce = p.commit_trace.read();
    CHECK(ce.exception && ce.pc == pc && ce.exc_cause == cause, "exception trace bad");
    CHECK(s.csr.instret == 0, "exception retired normally");
}

static void test_frontend_redirect_and_stale_drain() {
    BoomCoreState s;
    PipeSignals p;
    init_running(s);
    s.frontend.request_sent = true;
    s.frontend.pending_fetch_id = 9;
    s.frontend.pending_epoch = s.frontend.epoch;
    s.frontend.pending_address = 0x2000;
    s.frontend.halfword_valid = true;
    s.frontend.pending_packet.valid = true;
    s.frontend.fetch_buffer.count = 2;
    ImemResponse stale;
    stale.fetch_id = 9;
    stale.epoch = s.frontend.epoch;
    stale.address = 0x2000;
    p.imem_resp.write(stale);
    s.frontend_redirect.valid = true;
    s.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
    s.frontend_redirect.target_pc = BOOM_TRAP_VECTOR;
    s.exception_commit.valid = true;
    s.exception_commit.target = BOOM_TRAP_VECTOR;
    boom::frontend_module(s, p);
    CHECK(p.imem_resp.empty(), "stale response not drained");
    CHECK(s.frontend.epoch == 12, "epoch not advanced");
    CHECK(!s.frontend.halfword_valid, "cross-word carry survived");
    CHECK(!s.frontend.pending_packet.valid, "pending packet survived");
    CHECK(s.frontend.fetch_buffer.count == 0, "fetch buffer survived");
    CHECK(!s.frontend_redirect.valid, "redirect not consumed");
    CHECK(!p.imem_req.empty(), "trap request missing");
    ImemRequest req = p.imem_req.read();
    CHECK(req.address == BOOM_TRAP_VECTOR, "trap request address");
    CHECK(req.epoch == 12, "trap request epoch");
}

static void test_backpressure_atomicity() {
    BoomCoreState s;
    PipeSignals p;
    init_running(s);
    seed_exception(s, 0, 0x5000, 3, 0x00100073u, false);
    for (unsigned i = 0; i < 1024; i++) p.commit_trace.write(CommitEntry());
    boom::rob_commit_module(s, p);
    CHECK(!s.exception_commit.valid, "take occurred without trace capacity");
    CHECK(s.rob.entries[0].valid, "owner lost under backpressure");
    p.commit_trace.read();
    boom::rob_commit_module(s, p);
    CHECK(s.exception_commit.valid, "take did not retry");
}

static void test_stale_completion_rejected() {
    BoomCoreState s;
    PipeSignals p;
    init_running(s);
    seed_exception(s, 0, 0x7000, 2, 0xffffffffu, false);
    CompletionEvent stale;
    stale.valid = true;
    stale.kind = COMPLETION_EXECUTE;
    stale.source = COMPLETION_SOURCE_INT_EXECUTE;
    stale.uop = s.rob.entries[1].uop;
    stale.uop.rename.pdst = 41;
    stale.writes_prf = true;
    stale.value = 0xdeadbeef;
    boom::rob_commit_module(s, p);
    CHECK(!boom::apply_completion(s, stale), "stale completion accepted");
    CHECK(boom::prf_read(s, 41) == 0, "stale completion wrote PRF");
}

static void test_redirect_priorities() {
    BoomCoreState s;
    PipeSignals p;
    init_running(s);
    s.frontend_redirect.valid = true;
    s.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
    s.frontend_redirect.target_pc = BOOM_TRAP_VECTOR;
    s.exception_commit.valid = true;
    s.exception_commit.target = BOOM_TRAP_VECTOR;
    s.brupdate.valid = s.brupdate.mispredict = true;
    s.brupdate.jalr_target = 0x22220;
    boom::frontend_module(s, p);
    ImemRequest exception_req = p.imem_req.read();
    CHECK(exception_req.address == BOOM_TRAP_VECTOR,
          "younger branch beat architectural exception");

    BoomCoreState reset_state;
    PipeSignals reset_pipe;
    reset_state.frontend_redirect.valid = true;
    reset_state.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
    reset_state.frontend_redirect.target_pc = BOOM_TRAP_VECTOR;
    boom::frontend_module(reset_state, reset_pipe);
    ImemRequest reset_req = reset_pipe.imem_req.read();
    CHECK(reset_req.address == RESET_VECTOR, "exception beat runtime reset");
    CHECK(!reset_state.frontend_redirect.valid, "reset retained exception redirect");
}

int main() {
    for (int i = 0; i < 20; i++) {
        check_take(0x4000 + i * 4, 2, 0xffffffffu, false, 0xffffffffu,
                   (uint8_t)((ROB_DEPTH - 3 + i) % ROB_DEPTH));
        check_take(0x6000 + i * 2, 3, 0x00009002u, true, 0,
                   (uint8_t)((ROB_DEPTH - 2 + i) % ROB_DEPTH));
        check_take(0x8000 + i * 4, 1, 0, false, 0x8000 + i * 4,
                   (uint8_t)((ROB_DEPTH - 1 + i) % ROB_DEPTH));
    }
    test_frontend_redirect_and_stale_drain();
    test_backpressure_atomicity();
    test_stale_completion_rejected();
    test_redirect_priorities();
    std::cout << "PF1_DIRECTED checks=" << checks << " failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
