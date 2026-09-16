#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include "reset.hpp"
#include <cstdint>
#ifdef G6_L1R_NATIVE_DIAGNOSTIC
#include <cstdio>
#endif

#ifndef G6_L1R_FOCUSED_GROUP
#define G6_L1R_FOCUSED_GROUP 0
#endif

namespace boom {
bool lsu_accept_completion(BoomCoreState&, const MicroOp&, bool, bool, bool,
                           uint64_t, uint64_t, uint8_t, uint8_t);
bool lsu_reclaim_store(BoomCoreState&, SqIndex, uint16_t, uint8_t, uint32_t);
bool lsu_store_owner_matches(const BoomCoreState&, SqIndex, uint16_t, uint8_t, uint32_t);
void lsu_module(BoomCoreState&, PipeSignals&);
void branch_complete_event(BoomCoreState&, const MicroOp&, bool, uint64_t);
void rob_commit_module(BoomCoreState&, PipeSignals&);
void completion_service_cycle(BoomCoreState&, PipeSignals&);
}

enum FocusedCommand : uint8_t {
    F_INIT = 0, F_OBSERVE = 1, F_ALLOCATE = 2, F_LSU_STEP = 3,
    F_RESPONSE = 4, F_RECLAIM_STORE = 5, F_OWNER_MATCH = 6,
    F_BRANCH = 7, F_COMMIT = 8, F_RESET_BEGIN = 9, F_RESET_STEP = 10,
    F_PIPE_FILL = 11, F_PIPE_DRAIN = 12, F_MUTATE = 13, F_COMMIT_LSU = 14
};

enum FocusedMutation : uint8_t {
    M_SLOT_GENERATION = 0, M_ROB_OWNER = 1, M_ROB_FLAGS = 2,
    M_PENDING_OWNER = 3, M_QUEUE_OWNER = 4, M_GLOBAL_FLUSH = 5,
    M_NEXT_TRANSACTION = 6, M_PRF_SEED = 7, M_ROB_HEAD_TAIL = 8
};

static LsuState focused_lsu;
static RobInternalState focused_rob;
static BranchRecoveryState focused_branch_state;
static CompletionPendingState focused_completion;
static BranchUpdate focused_brupdate;
static ExceptionCommitEvent focused_exception_commit;
static FrontendRedirect focused_frontend_redirect;
static uint64_t focused_int_rf_bank0[INT_PHYS_REGS];
static uint64_t focused_int_rf_bank1[INT_PHYS_REGS];
static uint64_t focused_int_rf_latest_bank;
static bool focused_global_flush;
static PipeSignals focused_pipe;
static ResetControllerState focused_reset;
static DmemRequest focused_last_request;
static bool focused_last_request_valid;

static void focused_load_state(BoomCoreState& state) {
    state.lsu = focused_lsu;
    state.rob = focused_rob;
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 2 || G6_L1R_FOCUSED_GROUP == 3
    state.branch_state = focused_branch_state;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 1 || G6_L1R_FOCUSED_GROUP == 2 || G6_L1R_FOCUSED_GROUP == 3
    state.completion = focused_completion;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 2
    state.brupdate = focused_brupdate;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 3
    state.exception_commit = focused_exception_commit;
    state.frontend_redirect = focused_frontend_redirect;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 1 || G6_L1R_FOCUSED_GROUP == 2 || G6_L1R_FOCUSED_GROUP == 3
    state.int_rf_latest_bank = focused_int_rf_latest_bank;
    state.global_flush = focused_global_flush;
    for (int i = 0; i < INT_PHYS_REGS; ++i) {
        state.int_rf_bank0[i] = focused_int_rf_bank0[i];
        state.int_rf_bank1[i] = focused_int_rf_bank1[i];
    }
#endif
}

static void focused_save_state(const BoomCoreState& state) {
    focused_lsu = state.lsu;
    focused_rob = state.rob;
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 2 || G6_L1R_FOCUSED_GROUP == 3
    focused_branch_state = state.branch_state;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 1 || G6_L1R_FOCUSED_GROUP == 2 || G6_L1R_FOCUSED_GROUP == 3
    focused_completion = state.completion;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 2
    focused_brupdate = state.brupdate;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 3
    focused_exception_commit = state.exception_commit;
    focused_frontend_redirect = state.frontend_redirect;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 1 || G6_L1R_FOCUSED_GROUP == 2 || G6_L1R_FOCUSED_GROUP == 3
    focused_int_rf_latest_bank = state.int_rf_latest_bank;
    focused_global_flush = state.global_flush;
    for (int i = 0; i < INT_PHYS_REGS; ++i) {
        focused_int_rf_bank0[i] = state.int_rf_bank0[i];
        focused_int_rf_bank1[i] = state.int_rf_bank1[i];
    }
#endif
}

static void focused_initialize(BoomCoreState& focused_state) {
    focused_state.lsu = LsuState();
    for (int i = 0; i < LQ_DEPTH; ++i) focused_state.lsu.ldq[i] = LoadQueueEntry();
    for (int i = 0; i < SQ_DEPTH; ++i) focused_state.lsu.stq[i] = StoreQueueEntry();
    focused_state.rob = RobInternalState();
    focused_state.branch_state = BranchRecoveryState();
    focused_state.completion = CompletionPendingState();
    focused_state.brupdate = BranchUpdate();
    focused_state.global_flush = false;
    focused_state.exception_commit = ExceptionCommitEvent();
    focused_state.frontend_redirect = FrontendRedirect();
    focused_state.int_rf_latest_bank = 0;
    for (int i = 0; i < INT_PHYS_REGS; ++i) {
        focused_state.int_rf_bank0[i] = 0;
        focused_state.int_rf_bank1[i] = 0;
    }
    focused_reset = ResetControllerState();
    focused_last_request = DmemRequest();
    focused_last_request_valid = false;
    if (!focused_pipe.dmem_req.empty()) (void)focused_pipe.dmem_req.read();
    if (!focused_pipe.dmem_resp.empty()) (void)focused_pipe.dmem_resp.read();
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 3
    if (!focused_pipe.commit_trace.empty()) (void)focused_pipe.commit_trace.read();
#endif
}

static MicroOp focused_uop(uint8_t rob, uint32_t allocation, uint8_t branch_mask,
                           uint8_t pdst) {
    MicroOp uop;
    uop.queue.rob_idx = rob;
    uop.queue.rob_allocation_id = allocation;
    uop.branch.br_mask = branch_mask;
    if (pdst != 0) {
        uop.rename.dst_rtype = DST_INT;
        uop.rename.pdst = pdst;
        uop.rename.ldst = pdst & 31;
    }
    return uop;
}

static void focused_install(BoomCoreState& focused_state, const MicroOp& uop) {
    RobEntry& entry = focused_state.rob.entries[uop.queue.rob_idx];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.uop = uop;
}

static uint8_t focused_valid_lq(const BoomCoreState& focused_state) {
    uint8_t count = 0;
    for (int i = 0; i < LQ_DEPTH; ++i) if (focused_state.lsu.ldq[i].valid) ++count;
    return count;
}

static uint8_t focused_valid_sq(const BoomCoreState& focused_state) {
    uint8_t count = 0;
    for (int i = 0; i < SQ_DEPTH; ++i) if (focused_state.lsu.stq[i].valid) ++count;
    return count;
}

extern "C" void g6_l1r_focused_top(
    uint8_t opcode, uint8_t kind, uint8_t rob, uint8_t slot,
    uint16_t generation, uint8_t branch_mask, uint8_t mask, uint8_t size,
    uint8_t flags, uint8_t pdst, uint32_t allocation, uint32_t transaction,
    uint64_t address, uint64_t data,
    uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
    uint64_t& obs4, uint64_t& obs5) {
#pragma HLS INTERFACE ap_ctrl_hs port=return
#pragma HLS INTERFACE ap_none port=opcode
#pragma HLS INTERFACE ap_none port=kind
#pragma HLS INTERFACE ap_none port=rob
#pragma HLS INTERFACE ap_none port=slot
#pragma HLS INTERFACE ap_none port=generation
#pragma HLS INTERFACE ap_none port=branch_mask
#pragma HLS INTERFACE ap_none port=mask
#pragma HLS INTERFACE ap_none port=size
#pragma HLS INTERFACE ap_none port=flags
#pragma HLS INTERFACE ap_none port=pdst
#pragma HLS INTERFACE ap_none port=allocation
#pragma HLS INTERFACE ap_none port=transaction
#pragma HLS INTERFACE ap_none port=address
#pragma HLS INTERFACE ap_none port=data
#pragma HLS INTERFACE ap_none port=obs0
#pragma HLS INTERFACE ap_none port=obs1
#pragma HLS INTERFACE ap_none port=obs2
#pragma HLS INTERFACE ap_none port=obs3
#pragma HLS INTERFACE ap_none port=obs4
#pragma HLS INTERFACE ap_none port=obs5
#pragma HLS STREAM variable=focused_pipe.dmem_req depth=1
#pragma HLS STREAM variable=focused_pipe.dmem_resp depth=1
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 3
#pragma HLS STREAM variable=focused_pipe.commit_trace depth=1
#endif
    bool result = true;
    focused_last_request_valid = false;
    BoomCoreState focused_state;
    focused_load_state(focused_state);

    switch (opcode) {
    case F_INIT:
        focused_initialize(focused_state);
        break;
    case F_OBSERVE:
        break;
    case F_ALLOCATE: {
        MicroOp uop = focused_uop(rob, allocation, branch_mask, pdst);
        bool is_store = kind != 0;
        uop.ctrl.is_load = !is_store;
        uop.ctrl.is_sta = is_store;
        uop.mem.uses_ldq = !is_store;
        uop.mem.uses_stq = is_store;
        focused_install(focused_state, uop);
        result = boom::lsu_accept_completion(focused_state, uop, !is_store, is_store,
                                              (flags & 1) != 0, address, data, mask, size);
        break;
    }
#if G6_L1R_FOCUSED_GROUP != 4
    case F_LSU_STEP:
        boom::lsu_module(focused_state, focused_pipe);
        break;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 1 || G6_L1R_FOCUSED_GROUP == 2
    case F_RESPONSE: {
        DmemResponse response;
        response.transaction_id = transaction;
        response.data = data;
        response.read_data = (flags & 2) ? data : 0;
        response.exception = (flags & 1) != 0;
        response.exception_cause = response.exception ? address : 0;
        focused_pipe.dmem_resp.write(response);
        boom::completion_service_cycle(focused_state, focused_pipe);
        break;
    }
#endif
    case F_RECLAIM_STORE:
        result = slot < SQ_DEPTH && boom::lsu_reclaim_store(
            focused_state, (SqIndex)slot, generation, rob, allocation);
        break;
    case F_OWNER_MATCH:
        result = slot < SQ_DEPTH && boom::lsu_store_owner_matches(
            focused_state, (SqIndex)slot, generation, rob, allocation);
        break;
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 2
    case F_BRANCH: {
        MicroOp branch = focused_uop(rob, allocation, 0, 0);
        branch.branch.is_br = true;
        branch.branch.br_tag = slot & 7;
        focused_install(focused_state, branch);
        uint8_t bit = (uint8_t)(1u << (slot & 7));
        focused_state.branch_state.active_mask |= bit;
        focused_state.branch_state.tag_valid[slot & 7] = true;
        focused_state.branch_state.snapshot_valid[slot & 7] = true;
        boom::branch_complete_event(focused_state, branch, (flags & 1) != 0, address);
        break;
    }
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 3
    case F_COMMIT:
        boom::rob_commit_module(focused_state, focused_pipe);
        break;
#endif
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 4
    case F_RESET_BEGIN:
        focused_reset = ResetControllerState();
        break;
    case F_RESET_STEP:
        boom_core_reset_step(focused_state, focused_reset);
        break;
#endif
#if G6_L1R_FOCUSED_GROUP != 4
    case F_PIPE_FILL: {
        DmemRequest request;
        request.transaction_id = transaction;
        request.address = address;
        request.data = data;
        request.mask = mask;
        request.is_store = kind != 0;
        request.committed = (flags & 1) != 0;
        if (!focused_pipe.dmem_req.full()) focused_pipe.dmem_req.write(request);
        else result = false;
        break;
    }
    case F_PIPE_DRAIN:
        if (!focused_pipe.dmem_req.empty()) {
            focused_last_request = focused_pipe.dmem_req.read();
            focused_last_request_valid = true;
        } else result = false;
        break;
#endif
    case F_MUTATE:
        if (kind == M_SLOT_GENERATION && slot < LQ_DEPTH) {
            if (flags & 1) { focused_state.lsu.stq[slot].generation = generation; focused_state.lsu.stq_tail = slot; }
            else { focused_state.lsu.ldq[slot].generation = generation; focused_state.lsu.ldq_tail = slot; }
        } else if (kind == M_ROB_OWNER && rob < ROB_DEPTH) {
            focused_state.rob.entries[rob].uop.queue.rob_allocation_id = allocation;
        } else if (kind == M_ROB_FLAGS && rob < ROB_DEPTH) {
            RobEntry& entry = focused_state.rob.entries[rob];
            entry.valid = (flags & 1) != 0; entry.busy = (flags & 2) != 0;
            entry.exception = (flags & 4) != 0; entry.uop.exc_cause = address;
            entry.memory_request_sent = (flags & 8) != 0;
            entry.memory_completed = (flags & 16) != 0;
            entry.memory_transaction_id = transaction;
        } else if (kind == M_PENDING_OWNER) {
            focused_state.lsu.load_response_pending = (flags & 1) != 0;
            focused_state.lsu.pending_load_rob_idx = rob;
            focused_state.lsu.pending_load_allocation_id = allocation;
            focused_state.lsu.pending_load_lq_index = (LqIndex)slot;
            focused_state.lsu.pending_load_lq_generation = generation;
            focused_state.lsu.pending_load_transaction_id = transaction;
        } else if (kind == M_QUEUE_OWNER && slot < LQ_DEPTH) {
            LoadQueueEntry& entry = focused_state.lsu.ldq[slot];
            entry.valid = (flags & 1) != 0; entry.response_pending = (flags & 2) != 0;
            entry.rob_idx = rob; entry.rob_allocation_id = allocation;
            entry.generation = generation; entry.transaction_id = transaction;
        } else if (kind == M_GLOBAL_FLUSH) {
            focused_state.global_flush = (flags & 1) != 0;
        } else if (kind == M_NEXT_TRANSACTION) {
            focused_state.lsu.next_transaction_id = transaction;
        } else if (kind == M_PRF_SEED) {
            boom::prf_seed(focused_state, pdst, data);
        } else if (kind == M_ROB_HEAD_TAIL) {
            focused_state.rob.head = rob;
            focused_state.rob.tail = slot;
            focused_state.rob.maybe_full = (flags & 1) != 0;
        } else result = false;
        break;
#if G6_L1R_FOCUSED_GROUP == 0 || G6_L1R_FOCUSED_GROUP == 3
    case F_COMMIT_LSU:
        boom::rob_commit_module(focused_state, focused_pipe);
        boom::lsu_module(focused_state, focused_pipe);
        break;
#endif
    default:
        result = false;
        break;
    }

    focused_save_state(focused_state);

    uint8_t selected_lq = slot < LQ_DEPTH ? slot : 0;
    uint8_t selected_sq = slot < SQ_DEPTH ? slot : 0;
    uint8_t selected_rob = rob < ROB_DEPTH ? rob : 0;
    const LoadQueueEntry& lq = focused_state.lsu.ldq[selected_lq];
    const StoreQueueEntry& sq = focused_state.lsu.stq[selected_sq];
    const RobEntry& re = focused_state.rob.entries[selected_rob];
    obs0 = (uint64_t)result |
           ((uint64_t)focused_state.lsu.load_response_pending << 1) |
           ((uint64_t)focused_state.global_flush << 2) |
           ((uint64_t)focused_reset.completed << 3) |
           ((uint64_t)(focused_reset.phase & 15) << 4) |
           ((uint64_t)focused_state.lsu.ldq_count << 8) |
           ((uint64_t)focused_state.lsu.stq_count << 12) |
           ((uint64_t)focused_valid_lq(focused_state) << 16) |
           ((uint64_t)focused_valid_sq(focused_state) << 20) |
           ((uint64_t)focused_state.lsu.ldq_head << 24) |
           ((uint64_t)focused_state.lsu.ldq_tail << 28) |
           ((uint64_t)focused_state.lsu.stq_head << 32) |
           ((uint64_t)focused_state.lsu.stq_tail << 36) |
           ((uint64_t)focused_pipe.dmem_req.full() << 40) |
           ((uint64_t)focused_last_request_valid << 41) |
           ((uint64_t)focused_last_request.is_store << 42) |
           ((uint64_t)focused_last_request.committed << 43) |
           ((uint64_t)(focused_state.completion.completion_accepts_this_cycle != 0) << 44) |
           ((uint64_t)(focused_state.completion.rob_completes_this_cycle != 0) << 45) |
           ((uint64_t)(focused_state.completion.prf_writes_this_cycle != 0) << 46) |
           ((uint64_t)focused_state.rob.commit_valid << 47) |
           ((uint64_t)focused_state.exception_commit.valid << 48) |
           ((uint64_t)focused_state.brupdate.valid << 49) |
           ((uint64_t)lq.response_pending << 50) |
           ((uint64_t)re.memory_request_sent << 51) |
           ((uint64_t)re.memory_completed << 52) |
           ((uint64_t)re.valid << 53) |
           ((uint64_t)re.busy << 54) |
           ((uint64_t)lq.valid << 55) |
           ((uint64_t)sq.valid << 56);
    obs1 = (uint64_t)lq.generation | ((uint64_t)lq.rob_idx << 16) |
           ((uint64_t)lq.rob_allocation_id << 24) | ((uint64_t)lq.branch_mask << 56);
    obs2 = (uint64_t)sq.generation | ((uint64_t)sq.rob_idx << 16) |
           ((uint64_t)sq.rob_allocation_id << 24) | ((uint64_t)sq.branch_mask << 56);
    obs3 = (uint64_t)re.uop.queue.rob_allocation_id |
           ((uint64_t)re.memory_transaction_id << 32);
    obs4 = boom::prf_read(focused_state, pdst);
    obs5 = (uint64_t)focused_last_request.transaction_id |
           ((uint64_t)(uint32_t)focused_last_request.address << 32);
}

#ifdef G6_L1R_NATIVE_DIAGNOSTIC
static void native_command(uint8_t op, uint8_t kind, uint8_t rob, uint8_t slot,
                           uint16_t generation, uint8_t flags, uint32_t allocation,
                           uint32_t transaction, uint64_t address, uint64_t data,
                           uint64_t& o0, uint64_t& o1) {
    uint64_t o2, o3, o4, o5;
    g6_l1r_focused_top(op, kind, rob, slot, generation, 0, 0xff, 3, flags, 5,
                       allocation, transaction, address, data, o0, o1, o2, o3, o4, o5);
}

int main() {
    uint64_t o0, o1;
    native_command(F_INIT, 0, 0, 0, 0, 0, 0, 0, 0, 0, o0, o1);
    bool pass = ((o0 >> 8) & 0xff) == 0;
    native_command(F_MUTATE, M_SLOT_GENERATION, 0, 7, 0xfffe, 0, 0, 0, 0, 0, o0, o1);
    native_command(F_ALLOCATE, 0, 3, 7, 0, 0, 0x12345678, 0, 0x8000, 0, o0, o1);
    pass &= (o0 & 1) && ((o0 >> 8) & 15) == 1 && (uint16_t)o1 == 0xffff;
    native_command(F_LSU_STEP, 0, 3, 7, 0, 0, 0, 0, 0, 0, o0, o1);
    pass &= ((o0 >> 1) & 1);
    native_command(F_PIPE_DRAIN, 0, 3, 7, 0, 0, 0, 0, 0, 0, o0, o1);
    pass &= ((o0 >> 41) & 1);
    native_command(F_RESPONSE, 0, 3, 7, 0, 0, 0, 1, 0, 0x55, o0, o1);
    pass &= ((o0 >> 8) & 15) == 0 && ((o0 >> 45) & 1);
    native_command(F_INIT, 0, 0, 0, 0, 0, 0, 0, 0, 0, o0, o1);
    native_command(F_ALLOCATE, 1, 4, 0, 0, 0, 99, 0, 0x9000, 0xaa, o0, o1);
    uint16_t sq_generation = (uint16_t)o1;
    (void)sq_generation;
    pass &= ((o0 >> 12) & 15) == 1;
    std::printf("G6_L1R_COMMAND_ENGINE_%s commands=8\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
#endif
