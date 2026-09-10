#include "boom_config.hpp"
#include "boom_interfaces.hpp"
#include "boom_state.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

extern void boom_core_step(BoomCoreState&, PipeSignals&);

namespace {

const uint64_t kTohost = UINT64_C(0x80000080);

struct ExpectedRegister { uint8_t rd; uint64_t value; };
struct TestSpec {
    const char* name;
    std::vector<ExpectedRegister> signature;
    bool seed_taken;
    bool require_exception;
    unsigned min_predictions;
    unsigned min_predicted_taken;
    unsigned min_predicted_not_taken;
    unsigned min_correct;
    unsigned min_direction_mispredicts;
    unsigned min_redirects;
    unsigned min_wraps;
};

struct Coverage {
    unsigned conditional_predictions, predicted_taken, predicted_not_taken;
    unsigned correct_predictions, direction_mispredicts, target_mispredicts;
    unsigned recovery_redirects, same_packet_kills, ftq_squashes;
    unsigned fault_refetches, wraps, allocations, reclaims, exceptions;
    uint8_t last_tail;
    bool have_tail;
    Coverage() : conditional_predictions(0), predicted_taken(0),
        predicted_not_taken(0), correct_predictions(0), direction_mispredicts(0),
        target_mispredicts(0), recovery_redirects(0), same_packet_kills(0),
        ftq_squashes(0), fault_refetches(0), wraps(0), allocations(0),
        reclaims(0), exceptions(0), last_tail(0), have_tail(false) {}
    void sample(const BoomCoreState& state) {
        const boom::FtqStepOutput& ftq = state.ftq_last_output;
        allocations += ftq.alloc_accepted;
        reclaims += ftq.reclaimed;
        if (ftq.alloc_accepted && have_tail && ftq.alloc_ftq_idx < last_tail) ++wraps;
        if (ftq.alloc_accepted) { last_tail = ftq.alloc_ftq_idx; have_tail = true; }
        ftq_squashes += ftq.redirect_accepted;
        if (!state.brupdate.valid) return;
        if (state.brupdate.uop.branch.is_br) {
            ++conditional_predictions;
            if (state.brupdate.prediction_valid && state.brupdate.predicted_taken)
                ++predicted_taken;
            else
                ++predicted_not_taken;
            if (!state.brupdate.mispredict) ++correct_predictions;
            direction_mispredicts += state.brupdate.direction_mispredict;
            target_mispredicts += state.brupdate.target_mispredict;
            if (state.brupdate.mispredict && state.brupdate.uop.ftq_lane == 0)
                ++same_packet_kills;
        }
        recovery_redirects += state.brupdate.mispredict;
    }
};

std::string artifact(const char* name, const char* suffix) {
    const char* build = std::getenv("PF4_PROGRAM_BUILD");
    const std::string root = build && *build ? build : "/tmp/boom_hls/pf4/programs";
    return root + "/" + name + suffix;
}

bool load_image(const TestSpec& spec, std::vector<uint32_t>& words) {
    std::ifstream input(artifact(spec.name, ".bin").c_str(), std::ios::binary);
    if (!input) return false;
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                     std::istreambuf_iterator<char>());
    for (size_t i = 0; i < bytes.size(); i += 4) {
        uint32_t word = 0x00000013u;
        for (unsigned byte = 0; byte < 4 && i + byte < bytes.size(); ++byte) {
            word &= ~(UINT32_C(0xff) << (8 * byte));
            word |= static_cast<uint32_t>(bytes[i + byte]) << (8 * byte);
        }
        words.push_back(word);
    }
    return !words.empty();
}

std::vector<std::pair<uint64_t, unsigned> > load_initializers(const TestSpec& spec) {
    std::vector<std::pair<uint64_t, unsigned> > result;
    std::ifstream input(artifact(spec.name, ".init").c_str());
    std::string address;
    unsigned counter;
    while (input >> address >> counter) {
        std::istringstream parser(address);
        uint64_t offset = 0;
        parser >> std::hex >> offset;
        result.push_back(std::make_pair(RESET_VECTOR + offset, counter));
    }
    return result;
}

void train_counter(BoomCoreState& state, uint64_t pc, unsigned desired) {
    const unsigned count = desired == 3 ? 2 : (desired == 1 ? 0 : 1);
    const bool taken = desired >= 2;
    for (unsigned i = 0; i < count; ++i) {
        boom::PredictorStepInput input;
        input.active_generation = state.predictor_generation;
        input.update.valid = true;
        input.update.commit_qualified = true;
        input.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
        input.update.pc = pc;
        input.update.metadata_token = static_cast<uint16_t>((pc >> 1) & 255u);
        input.update.generation = state.predictor_generation;
        input.update.taken = taken;
        state.predictor.step(input);
    }
}

bool probe_counter(BoomCoreState& state, uint64_t pc) {
    boom::PredictorStepInput drain;
    drain.active_generation = state.predictor_generation;
    drain.resp_ready = true;
    state.predictor.step(drain);
    boom::PredictorStepInput request;
    request.active_generation = state.predictor_generation;
    request.req_valid = true;
    request.request.pc = pc;
    request.request.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    request.request.generation = state.predictor_generation;
    state.predictor.step(request);
    boom::PredictorStepInput consume;
    consume.active_generation = state.predictor_generation;
    consume.resp_ready = true;
    return state.predictor.step(consume).response.taken;
}

struct PendingImem { ImemRequest request; unsigned ready_cycle; };
struct InstructionMemory {
    const std::vector<uint32_t>& words;
    std::vector<PendingImem> pending;
    bool inject_fault;
    bool fault_sent;
    explicit InstructionMemory(const std::vector<uint32_t>& image, bool fault)
        : words(image), inject_fault(fault), fault_sent(false) {}
    void step(PipeSignals& pipe, unsigned cycle) {
        while (!pipe.imem_req.empty())
            pending.push_back(PendingImem{pipe.imem_req.read(), cycle + 2});
        if (pending.empty() || pending.front().ready_cycle > cycle ||
            pipe.imem_resp.full()) return;
        const ImemRequest request = pending.front().request;
        pending.erase(pending.begin());
        ImemResponse response;
        response.address = request.address;
        response.fetch_id = request.fetch_id;
        response.epoch = request.epoch;
        const uint64_t index = request.address >= RESET_VECTOR ?
            (request.address - RESET_VECTOR) >> 2 : UINT64_MAX;
        response.instruction = index < words.size() ? words[index] : 0x00000013u;
        if (inject_fault && !fault_sent && request.address == RESET_VECTOR + 4) {
            response.exception = true;
            response.exc_cause = 1;
            fault_sent = true;
        }
        pipe.imem_resp.write(response);
    }
};

struct PendingLoad { DmemRequest request; unsigned ready_cycle; };
struct DataMemory {
    std::map<uint64_t, uint8_t> bytes;
    std::vector<PendingLoad> pending;
    bool saw_tohost;
    uint64_t tohost_value;
    DataMemory() : saw_tohost(false), tohost_value(0) {}
    uint64_t read64(uint64_t address) const {
        uint64_t value = 0;
        for (unsigned i = 0; i < 8; ++i) {
            std::map<uint64_t, uint8_t>::const_iterator it = bytes.find(address + i);
            if (it != bytes.end()) value |= static_cast<uint64_t>(it->second) << (8 * i);
        }
        return value;
    }
    void step(PipeSignals& pipe, unsigned cycle) {
        if (!pending.empty() && pending.front().ready_cycle <= cycle &&
            !pipe.dmem_resp.full()) {
            DmemResponse response;
            response.transaction_id = pending.front().request.transaction_id;
            response.data = response.read_data = read64(pending.front().request.address);
            pipe.dmem_resp.write(response);
            pending.erase(pending.begin());
        }
        while (!pipe.dmem_req.empty()) {
            const DmemRequest request = pipe.dmem_req.read();
            if (!request.is_store) {
                pending.push_back(PendingLoad{request, cycle + 3});
                continue;
            }
            const uint8_t mask = request.write_mask ? request.write_mask : request.mask;
            for (unsigned i = 0; i < 8; ++i)
                if (mask & (1u << i)) bytes[request.address + i] =
                    static_cast<uint8_t>(request.write_data >> (8 * i));
            if (request.address == kTohost) {
                saw_tohost = true;
                tohost_value = request.write_data;
            }
        }
    }
};

bool run_test(const TestSpec& spec) {
    std::vector<uint32_t> words;
    if (!load_image(spec, words)) return false;
    BoomCoreState state;
    PipeSignals pipe;
    state.product_ftq_enabled = true;
    const std::vector<std::pair<uint64_t, unsigned> > initializers =
        load_initializers(spec);
    std::vector<bool> before;
    for (size_t i = 0; i < initializers.size(); ++i) {
        train_counter(state, initializers[i].first, initializers[i].second);
        before.push_back(probe_counter(state, initializers[i].first));
    }
    InstructionMemory imem(words, std::string(spec.name) == "pf4_fault_refetch");
    DataMemory dmem;
    Coverage coverage;
    uint64_t registers[32] = {};
    bool written[32] = {};
    unsigned commits = 0;
    unsigned cycle = 0;
    for (; cycle < 30000 && !dmem.saw_tohost; ++cycle) {
        imem.step(pipe, cycle);
        dmem.step(pipe, cycle);
        boom_core_step(state, pipe);
        coverage.sample(state);
        while (!pipe.commit_trace.empty()) {
            const CommitEntry entry = pipe.commit_trace.read();
            ++commits;
            if (entry.rd_valid && entry.rd < 32) {
                registers[entry.rd] = entry.rd_value;
                written[entry.rd] = true;
            }
            if (entry.exception) ++coverage.exceptions;
        }
        dmem.step(pipe, cycle);
    }
    if (spec.require_exception && coverage.exceptions != 0 && imem.fault_sent)
        coverage.fault_refetches = 1;
    bool bim_preserved = true;
    for (size_t i = 0; i < initializers.size(); ++i)
        bim_preserved = bim_preserved &&
            probe_counter(state, initializers[i].first) == before[i];
    bool ok = dmem.saw_tohost && dmem.tohost_value == 1 && !state.io_trap &&
        coverage.allocations > 0 && coverage.reclaims > 0 && bim_preserved &&
        coverage.exceptions >= (spec.require_exception ? 1u : 0u) &&
        coverage.conditional_predictions >= spec.min_predictions &&
        coverage.predicted_taken >= spec.min_predicted_taken &&
        coverage.predicted_not_taken >= spec.min_predicted_not_taken &&
        coverage.correct_predictions >= spec.min_correct &&
        coverage.direction_mispredicts >= spec.min_direction_mispredicts &&
        coverage.recovery_redirects >= spec.min_redirects &&
        coverage.wraps >= spec.min_wraps;
    for (size_t i = 0; i < spec.signature.size(); ++i)
        ok = ok && written[spec.signature[i].rd] &&
            registers[spec.signature[i].rd] == spec.signature[i].value;
    if (!ok) {
        for (size_t i = 0; i < spec.signature.size(); ++i)
            std::printf("PF4_SIGNATURE_DETAIL program=%s rd=%u written=%u actual=%llu expected=%llu\n",
                spec.name, spec.signature[i].rd, written[spec.signature[i].rd],
                static_cast<unsigned long long>(registers[spec.signature[i].rd]),
                static_cast<unsigned long long>(spec.signature[i].value));
    }
    std::printf("PF4_PROGRAM program=%s conditional_predictions=%u predicted_taken=%u "
        "predicted_not_taken=%u correct_predictions=%u direction_mispredicts=%u "
        "target_mispredicts=%u recovery_redirects=%u same_packet_kills=%u "
        "ftq_squashes=%u fault_refetches=%u wraps=%u signature=%s "
        "bim_preserved=%s commits=%u cycles=%u verdict=%s\n", spec.name,
        coverage.conditional_predictions, coverage.predicted_taken,
        coverage.predicted_not_taken, coverage.correct_predictions,
        coverage.direction_mispredicts, coverage.target_mispredicts,
        coverage.recovery_redirects, coverage.same_packet_kills,
        coverage.ftq_squashes, coverage.fault_refetches, coverage.wraps,
        ok ? "PASS" : "FAIL", bim_preserved ? "true" : "false", commits,
        cycle, ok ? "PASS" : "FAIL");
    return ok;
}

}  // namespace

int main() {
    const std::vector<TestSpec> tests = {
        {"pf4_pred_nt_actual_nt", {{8,11},{9,21},{18,31}}, false,false,1,0,1,1,0,0,0},
        {"pf4_pred_nt_actual_t", {{8,0},{9,22},{18,32}}, false,false,1,0,1,0,1,1,0},
        {"pf4_pred_t_actual_t", {{8,7},{9,23},{18,34}}, true,false,1,1,0,1,0,0,0},
        {"pf4_pred_t_actual_nt", {{8,5},{9,24},{18,35}}, true,false,1,1,0,0,1,1,0},
        {"pf4_rvc_mispredict", {{8,6},{9,25},{18,37}}, false,false,1,0,1,0,1,1,0},
        {"pf4_same_packet_kill", {{8,0},{9,26},{18,13},{19,39}}, false,false,1,0,1,0,1,1,0},
        {"pf4_fault_refetch", {{8,27},{9,37},{18,47}}, false,true,0,0,0,0,0,0,0},
        {"pf4_ftq_wrap_recovery", {{8,80},{9,1},{18,88}}, false,false,1,0,1,0,1,1,1},
        {"pf4_jal_preservation", {{8,29},{9,39},{18,49}}, false,false,0,0,0,0,0,0,0},
        {"pf4_jalr_unpredicted", {{8,30},{9,40},{18,50}}, false,false,0,0,0,0,0,1,0},
        {"pf4_exception_priority", {{8,31},{9,41},{18,51}}, false,true,0,0,0,0,0,0,0},
        {"pf4_mixed_long_control", {{8,12},{18,52},{19,62}}, false,false,12,0,12,1,11,11,0}
    };
    unsigned passed = 0;
    for (size_t i = 0; i < tests.size(); ++i) passed += run_test(tests[i]);
    std::printf("PF4_PRODUCT_PROGRAMS %u/%u %s\n", passed,
                static_cast<unsigned>(tests.size()),
                passed == tests.size() ? "PASS" : "FAIL");
    return passed == tests.size() ? 0 : 1;
}
