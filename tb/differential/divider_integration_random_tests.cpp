#include "boom_state.hpp"
#include "completion.hpp"
#include "divider.hpp"

#include <climits>
#include <cstdint>
#include <iostream>
#include <map>

namespace boom {
void issue_module(BoomCoreState& state);
void execute_module(BoomCoreState& state);
void branch_complete_event(BoomCoreState& state, const MicroOp& uop,
                           bool mispredict, uint64_t redirect_pc);
}

using namespace boom;

static const unsigned kSeeds = 128;
static const unsigned kCycles = 1024;

static uint64_t next_random(uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

static uint64_t sext32(uint32_t value) {
    return (value & 0x80000000U) ? 0xffffffff00000000ULL | value : value;
}

static uint64_t divide_reference(DivideOperation op, uint64_t lhs,
                                 uint64_t rhs) {
    const bool word = op >= DIVW_OP_SIGNED;
    const bool sign = op == DIV_OP_SIGNED || op == REM_OP_SIGNED ||
                      op == DIVW_OP_SIGNED || op == REMW_OP_SIGNED;
    const bool rem = op == REM_OP_SIGNED || op == REM_OP_UNSIGNED ||
                     op == REMW_OP_SIGNED || op == REMW_OP_UNSIGNED;
    if (word) {
        const uint32_t a = static_cast<uint32_t>(lhs);
        const uint32_t b = static_cast<uint32_t>(rhs);
        uint32_t result;
        if (b == 0) result = rem ? a : UINT32_MAX;
        else if (sign) {
            const int32_t sa = static_cast<int32_t>(a);
            const int32_t sb = static_cast<int32_t>(b);
            if (sa == INT32_MIN && sb == -1) result = rem ? 0 : a;
            else result = static_cast<uint32_t>(rem ? sa % sb : sa / sb);
        } else {
            result = rem ? a % b : a / b;
        }
        return sext32(result);
    }
    if (rhs == 0) return rem ? lhs : UINT64_MAX;
    if (sign) {
        const int64_t a = static_cast<int64_t>(lhs);
        const int64_t b = static_cast<int64_t>(rhs);
        if (a == INT64_MIN && b == -1) return rem ? 0 : lhs;
        return static_cast<uint64_t>(rem ? a % b : a / b);
    }
    return rem ? lhs % rhs : lhs / rhs;
}

struct Token {
    bool divider;
    bool accepted;
    bool live;
    bool responded;
    bool completed;
    bool wrote_back;
    uint8_t rob_idx;
    uint8_t pdst;
    uint64_t expected;

    Token() : divider(false), accepted(false), live(false), responded(false),
        completed(false), wrote_back(false), rob_idx(0), pdst(0), expected(0) {}
};

struct Metrics {
    uint64_t op[8];
    uint64_t accepted_dividers;
    uint64_t divider_responses;
    uint64_t divider_completes;
    uint64_t canceled;
    uint64_t alu;
    uint64_t mul;
    uint64_t branch_kills;
    uint64_t global_flushes;
    uint64_t direct_cancels;
    uint64_t stale_reuses;
    uint64_t x0;
    uint64_t held_response_checks;
    uint64_t completion_backpressure_cycles;
    uint64_t dropped;
    uint64_t duplicate_response;
    uint64_t duplicate_writeback;
    uint64_t duplicate_rob_complete;
    uint64_t stale_side_effect;
    uint64_t response_instability;
    uint64_t value_mismatch;

    Metrics() : accepted_dividers(0), divider_responses(0),
        divider_completes(0), canceled(0), alu(0), mul(0), branch_kills(0),
        global_flushes(0), direct_cancels(0), stale_reuses(0), x0(0),
        held_response_checks(0), completion_backpressure_cycles(0), dropped(0),
        duplicate_response(0), duplicate_writeback(0),
        duplicate_rob_complete(0), stale_side_effect(0),
        response_instability(0), value_mismatch(0) {
        for (unsigned i = 0; i < 8; ++i) op[i] = 0;
    }
};

static int free_rob_slot(const BoomCoreState& state) {
    for (int i = 0; i < ROB_DEPTH; ++i)
        if (!state.rob.entries[i].valid) return i;
    return -1;
}

static void cancel_token(std::map<uint32_t, Token>& tokens, uint32_t id,
                         Metrics& metrics) {
    std::map<uint32_t, Token>::iterator it = tokens.find(id);
    if (it != tokens.end() && it->second.accepted && it->second.live) {
        it->second.live = false;
        if (it->second.divider) ++metrics.canceled;
    }
}

static void cancel_invalid_owners(const BoomCoreState& state,
                                  std::map<uint32_t, Token>& tokens,
                                  Metrics& metrics) {
    for (std::map<uint32_t, Token>::iterator it = tokens.begin();
         it != tokens.end(); ++it) {
        Token& token = it->second;
        if (!token.accepted || !token.live) continue;
        const RobEntry& owner = state.rob.entries[token.rob_idx];
        if (!owner.valid || owner.uop.queue.rob_allocation_id != it->first) {
            token.live = false;
            if (token.divider) ++metrics.canceled;
        }
    }
}

static MicroOp make_uop(uint8_t uopc, uint8_t fu, uint8_t rob_idx,
                        uint32_t allocation_id, uint8_t pdst,
                        uint8_t branch_mask) {
    MicroOp uop;
    uop.uopc = uopc;
    uop.fu_code = fu;
    uop.iq_type = IQ_ALU;
    uop.ctrl.op1_sel = OP1_RS1;
    uop.ctrl.op2_sel = OP2_RS2;
    uop.rename.prs1 = 1;
    uop.rename.prs2 = 2;
    uop.rename.pdst = pdst;
    uop.rename.dst_rtype = pdst == 0 ? DST_X0 : DST_INT;
    uop.queue.rob_idx = rob_idx;
    uop.queue.rob_allocation_id = allocation_id;
    uop.branch.br_mask = branch_mask;
    return uop;
}

static void observe_new_result(const ExecuteState::AluResult& before,
                               const ExecuteState::AluResult& after,
                               std::map<uint32_t, Token>& tokens,
                               Metrics& metrics) {
    if (!after.valid) return;
    const uint32_t id = after.uop.queue.rob_allocation_id;
    if (before.valid && before.uop.queue.rob_allocation_id == id) return;
    std::map<uint32_t, Token>::iterator it = tokens.find(id);
    if (it == tokens.end() || !it->second.live) {
        ++metrics.stale_side_effect;
        return;
    }
    Token& token = it->second;
    if (after.result != token.expected) ++metrics.value_mismatch;
    if (token.divider) {
        if (token.responded) ++metrics.duplicate_response;
        token.responded = true;
        ++metrics.divider_responses;
    }
}

static void observe_completion(BoomCoreState& state,
                               std::map<uint32_t, Token>& tokens,
                               Metrics& metrics) {
    for (int port = 0; port < NUM_INT_WRITEBACK_PORTS; ++port) {
        const WritebackEvent& wb = state.completion.writebacks[port];
        if (!wb.valid) continue;
        std::map<uint32_t, Token>::iterator it =
            tokens.find(wb.rob_allocation_id);
        if (it == tokens.end() || !it->second.live) {
            ++metrics.stale_side_effect;
            continue;
        }
        Token& token = it->second;
        if (token.wrote_back) ++metrics.duplicate_writeback;
        token.wrote_back = true;
        if (wb.pdst != token.pdst || wb.value != token.expected)
            ++metrics.value_mismatch;
    }
    for (std::map<uint32_t, Token>::iterator it = tokens.begin();
         it != tokens.end(); ++it) {
        Token& token = it->second;
        if (!token.accepted || !token.live) continue;
        const RobEntry& owner = state.rob.entries[token.rob_idx];
        if (!owner.valid || owner.uop.queue.rob_allocation_id != it->first)
            continue;
        if (!owner.busy) {
            if (token.completed) ++metrics.duplicate_rob_complete;
            token.completed = true;
            token.live = false;
            if (token.divider) ++metrics.divider_completes;
            if (token.pdst != 0 && !token.wrote_back)
                ++metrics.stale_side_effect;
        }
    }
}

int main() {
    Metrics metrics;
    uint32_t next_allocation = 1;
    unsigned next_div_op = 0;

    for (unsigned seed = 0; seed < kSeeds; ++seed) {
        BoomCoreState state;
        std::map<uint32_t, Token> tokens;
        uint64_t rng = 0xa0761d6478bd642fULL ^
            (static_cast<uint64_t>(seed) * 0xe7037ed1a0b428dbULL);
        unsigned completion_hold = 0;
        bool held_valid = false;
        uint64_t held_value = 0;

        for (unsigned cycle = 0; cycle < kCycles; ++cycle) {
            state.brupdate = BranchUpdate();
            state.global_flush = false;
            const uint64_t control = next_random(rng);

            const DividerResponse response_before =
                divider_response(state.execute.divider.arithmetic);
            const bool blocked_response = response_before.valid &&
                state.execute.divider.token_valid &&
                state.execute.alu_results[INT_ISSUE_LANE].valid;
            if (blocked_response) {
                ++metrics.held_response_checks;
                if (held_valid && response_before.result != held_value)
                    ++metrics.response_instability;
                held_valid = true;
                held_value = response_before.result;
            } else {
                held_valid = false;
            }

            const bool active_divider = state.execute.divider.token_valid;
            if (active_divider && (control & 0x1ffU) == 0) {
                const uint32_t id = state.execute.divider.allocation_id;
                cancel_token(tokens, id, metrics);
                divider_reset(state.execute.divider.arithmetic);
                state.execute.divider = DividerExecutionState();
                ++metrics.direct_cancels;
            } else if (active_divider && (control & 0x3ffU) == 1) {
                const uint32_t id = state.execute.divider.allocation_id;
                cancel_token(tokens, id, metrics);
                state.global_flush = true;
                ++metrics.global_flushes;
            } else if (active_divider && (control & 0x3ffU) == 2) {
                const uint32_t id = state.execute.divider.allocation_id;
                const uint8_t index = state.execute.divider.rob_idx;
                cancel_token(tokens, id, metrics);
                RobEntry replacement;
                replacement.valid = replacement.busy = true;
                replacement.uop.queue.rob_idx = index;
                replacement.uop.queue.rob_allocation_id = next_allocation++;
                state.rob.entries[index] = replacement;
                ++metrics.stale_reuses;
            } else if (active_divider && (control & 0x1ffU) == 3) {
                const int branch_slot = free_rob_slot(state);
                if (branch_slot >= 0) {
                    MicroOp branch = make_uop(31, FU_ALU,
                        static_cast<uint8_t>(branch_slot), next_allocation++, 0, 0);
                    branch.branch.is_br = true;
                    branch.branch.br_tag = 0;
                    RobEntry& owner = state.rob.entries[branch_slot];
                    owner = RobEntry();
                    owner.valid = owner.busy = true;
                    owner.uop = branch;
                    state.branch_state.active_mask = 1;
                    state.branch_state.tag_valid[0] = true;
                    state.branch_state.snapshot_valid[0] = true;
                    branch_complete_event(state, branch, true, 0x1000 + cycle * 4);
                    ++metrics.branch_kills;
                }
            }

            cancel_invalid_owners(state, tokens, metrics);

            const bool result_slot_free =
                !state.execute.alu_results[INT_ISSUE_LANE].valid;
            const bool divider_response_waiting =
                divider_response(state.execute.divider.arithmetic).valid;
            state.issue.port_ready[INT_ISSUE_LANE] =
                result_slot_free && !divider_response_waiting &&
                !state.global_flush && !state.brupdate.valid;

            bool proposed = false;
            uint32_t proposed_id = 0;
            if (state.issue.port_ready[INT_ISSUE_LANE] &&
                !state.rename.dispatch_packets[0].valid &&
                (control & 3U) != 0) {
                const int slot = free_rob_slot(state);
                if (slot >= 0) {
                    const bool want_divider = !state.execute.divider.token_valid &&
                        (control & 7U) != 0;
                    const uint64_t lhs = next_random(rng);
                    uint64_t rhs = next_random(rng);
                    if ((control & 0x7fU) == 0) rhs = 0;
                    const uint8_t pdst = ((control >> 8) & 7U) == 0 ? 0 :
                        static_cast<uint8_t>(3 + ((control >> 12) % 48));
                    uint8_t uopc;
                    uint8_t fu;
                    uint64_t expected;
                    bool is_divider = false;
                    if (want_divider) {
                        const unsigned op = next_div_op++ & 7U;
                        uopc = static_cast<uint8_t>(21 + op);
                        fu = FU_DIV;
                        expected = divide_reference(
                            static_cast<DivideOperation>(op), lhs, rhs);
                        is_divider = true;
                    } else if ((control & 0x18U) == 0x18U) {
                        uopc = 16;
                        fu = FU_MUL;
                        expected = lhs * rhs;
                    } else {
                        static const uint8_t alu_ops[4] = {1, 6, 9, 10};
                        uopc = alu_ops[(control >> 5) & 3U];
                        fu = FU_ALU;
                        if (uopc == 1) expected = lhs + rhs;
                        else if (uopc == 6) expected = lhs ^ rhs;
                        else if (uopc == 9) expected = lhs | rhs;
                        else expected = lhs & rhs;
                    }
                    proposed_id = next_allocation++;
                    const uint8_t branch_mask = is_divider ? 1 : 0;
                    const MicroOp uop = make_uop(uopc, fu,
                        static_cast<uint8_t>(slot), proposed_id, pdst,
                        branch_mask);
                    RobEntry& owner = state.rob.entries[slot];
                    owner = RobEntry();
                    owner.valid = owner.busy = true;
                    owner.uop = uop;
                    state.rename.int_free_list.busy_table[pdst] = pdst != 0;
                    prf_seed(state, 1, lhs);
                    prf_seed(state, 2, rhs);
                    RenameDispatchPacket& packet =
                        state.rename.dispatch_packets[0];
                    packet.valid = packet.rob_allocated = true;
                    packet.uop = uop;
                    Token token;
                    token.divider = is_divider;
                    token.rob_idx = static_cast<uint8_t>(slot);
                    token.pdst = pdst;
                    token.expected = expected;
                    tokens[proposed_id] = token;
                    proposed = true;
                    if (pdst == 0) ++metrics.x0;
                }
            }

            issue_module(state);
            const bool issued = state.issue.issued_valids[INT_ISSUE_LANE];
            const uint32_t issued_id = issued ?
                state.issue.issued_uops[INT_ISSUE_LANE].queue.rob_allocation_id : 0;
            if (proposed && issued_id != proposed_id) ++metrics.value_mismatch;

            const ExecuteState::AluResult result_before =
                state.execute.alu_results[INT_ISSUE_LANE];
            const bool divider_before = state.execute.divider.token_valid;
            execute_module(state);

            if (issued) {
                Token& token = tokens[issued_id];
                token.accepted = true;
                token.live = true;
                if (token.divider) {
                    if (divider_before || !state.execute.divider.token_valid)
                        ++metrics.value_mismatch;
                    ++metrics.accepted_dividers;
                    ++metrics.op[state.issue.issued_uops[INT_ISSUE_LANE].uopc - 21];
                } else if (state.issue.issued_uops[INT_ISSUE_LANE].fu_code == FU_MUL) {
                    ++metrics.mul;
                } else {
                    ++metrics.alu;
                }
            }
            observe_new_result(result_before,
                               state.execute.alu_results[INT_ISSUE_LANE],
                               tokens, metrics);

            if (state.global_flush) cancel_invalid_owners(state, tokens, metrics);
            if (state.execute.alu_results[INT_ISSUE_LANE].valid &&
                completion_hold == 0 && (control & 0x30U) == 0)
                completion_hold = 1 + static_cast<unsigned>((control >> 16) & 7U);
            if (completion_hold != 0) {
                --completion_hold;
                ++metrics.completion_backpressure_cycles;
            } else {
                completion_service_execute(state);
                observe_completion(state, tokens, metrics);
            }
            cancel_invalid_owners(state, tokens, metrics);

            for (int i = 0; i < ROB_DEPTH; ++i) {
                RobEntry& entry = state.rob.entries[i];
                if (entry.valid && !entry.busy) entry = RobEntry();
                else if (entry.valid &&
                         tokens.find(entry.uop.queue.rob_allocation_id) == tokens.end() &&
                         (!state.execute.divider.token_valid ||
                          state.execute.divider.rob_idx != i))
                    entry = RobEntry();
            }
            prf_force_x0(state);
        }

        if (state.execute.divider.token_valid) {
            cancel_token(tokens, state.execute.divider.allocation_id, metrics);
            divider_reset(state.execute.divider.arithmetic);
            state.execute.divider = DividerExecutionState();
        }
        for (std::map<uint32_t, Token>::iterator it = tokens.begin();
             it != tokens.end(); ++it)
            if (it->second.accepted && it->second.live) {
                it->second.live = false;
                if (it->second.divider) ++metrics.canceled;
            }
    }

    metrics.dropped = metrics.accepted_dividers -
        metrics.divider_completes - metrics.canceled;
    uint64_t failures = metrics.dropped + metrics.duplicate_response +
        metrics.duplicate_writeback + metrics.duplicate_rob_complete +
        metrics.stale_side_effect + metrics.response_instability +
        metrics.value_mismatch;
    for (unsigned op = 0; op < 8; ++op)
        if (metrics.op[op] == 0) ++failures;
    if (metrics.branch_kills == 0 || metrics.global_flushes == 0 ||
        metrics.direct_cancels == 0 || metrics.stale_reuses == 0 ||
        metrics.held_response_checks == 0 || metrics.alu == 0 ||
        metrics.mul == 0 || metrics.x0 == 0)
        ++failures;

    std::cout << "Gate 4.1 M3B divider integration random: "
              << (failures == 0 ? "PASS" : "FAIL") << "\n"
              << "seeds=" << kSeeds << "\ncycles_per_seed=" << kCycles
              << "\ntotal_cycles=" << static_cast<uint64_t>(kSeeds) * kCycles << "\n";
    for (unsigned op = 0; op < 8; ++op)
        std::cout << "divider_op_" << op << "=" << metrics.op[op] << "\n";
    std::cout << "accepted_divider_tokens=" << metrics.accepted_dividers
              << "\ndivider_responses=" << metrics.divider_responses
              << "\ndivider_rob_completes=" << metrics.divider_completes
              << "\ncanceled_tokens=" << metrics.canceled
              << "\nordinary_alu=" << metrics.alu
              << "\nordinary_mul=" << metrics.mul
              << "\nbranch_kills=" << metrics.branch_kills
              << "\nglobal_flushes=" << metrics.global_flushes
              << "\ndirect_divider_cancels=" << metrics.direct_cancels
              << "\nstale_allocation_reuses=" << metrics.stale_reuses
              << "\nrd_x0=" << metrics.x0
              << "\nheld_response_checks=" << metrics.held_response_checks
              << "\ncompletion_backpressure_cycles="
              << metrics.completion_backpressure_cycles
              << "\ndropped=" << metrics.dropped
              << "\nduplicate_response=" << metrics.duplicate_response
              << "\nduplicate_writeback=" << metrics.duplicate_writeback
              << "\nduplicate_rob_complete=" << metrics.duplicate_rob_complete
              << "\nstale_side_effect=" << metrics.stale_side_effect
              << "\nresponse_instability=" << metrics.response_instability
              << "\nvalue_mismatch=" << metrics.value_mismatch << "\n";
    return failures == 0 ? 0 : 1;
}
