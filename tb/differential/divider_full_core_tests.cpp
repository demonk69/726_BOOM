#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static const uint64_t TOHOST = UINT64_C(0x80000080);

struct ExpectedRegister {
    uint8_t rd;
    uint64_t value;
};

struct TestSpec {
    const char* name;
    std::vector<ExpectedRegister> expected;
    unsigned encoded_dividers;
    unsigned committed_dividers;
    unsigned minimum_busy_cycles;
    bool needs_load;
    bool branch_kill;
};

static bool is_divider_instruction(uint32_t inst) {
    const uint32_t opcode = inst & 0x7f;
    const uint32_t funct3 = (inst >> 12) & 7;
    return (opcode == 0x33 || opcode == 0x3b) &&
           ((inst >> 25) & 0x7f) == 1 && funct3 >= 4;
}

static std::string program_path(const char* name) {
    std::string source = __FILE__;
    const std::string marker = "/tb/differential/";
    const std::string::size_type at = source.rfind(marker);
    if (at != std::string::npos)
        return source.substr(0, at) + "/tb/programs/boom_reference/m3b/" + name + ".hex";
    return std::string("tb/programs/boom_reference/m3b/") + name + ".hex";
}

static bool load_program(const TestSpec& spec, std::vector<uint32_t>& words) {
    const std::string path = program_path(spec.name);
    std::ifstream input(path.c_str());
    if (!input) {
        std::printf("FAIL %-22s cannot open %s\n", spec.name, path.c_str());
        return false;
    }
    std::string line;
    while (std::getline(input, line)) {
        const std::string::size_type comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        std::istringstream fields(line);
        std::string token;
        while (fields >> token) {
            char* end = 0;
            const uint64_t packed = std::strtoull(token.c_str(), &end, 16);
            if (!end || *end != '\0' || token.size() > 16) {
                std::printf("FAIL %-22s malformed hex token %s\n", spec.name, token.c_str());
                return false;
            }
            words.push_back((uint32_t)packed);
            if (token.size() > 8) words.push_back((uint32_t)(packed >> 32));
        }
    }
    unsigned dividers = 0;
    for (size_t i = 0; i < words.size(); ++i)
        if (is_divider_instruction(words[i])) ++dividers;
    if (dividers != spec.encoded_dividers) {
        std::printf("FAIL %-22s encoded divider count %u, expected %u\n",
                    spec.name, dividers, spec.encoded_dividers);
        return false;
    }
    return true;
}

struct PendingImem {
    ImemRequest request;
    unsigned ready_cycle;
};

struct InstructionMemory {
    const std::vector<uint32_t>& words;
    std::vector<PendingImem> pending;

    explicit InstructionMemory(const std::vector<uint32_t>& program) : words(program) {}

    void step(PipeSignals& pipe, unsigned cycle) {
        while (!pipe.imem_req.empty()) {
            PendingImem item;
            item.request = pipe.imem_req.read();
            item.ready_cycle = cycle + 2;
            pending.push_back(item);
        }
        if (!pending.empty() && pending.front().ready_cycle <= cycle && !pipe.imem_resp.full()) {
            const ImemRequest request = pending.front().request;
            pending.erase(pending.begin());
            ImemResponse response;
            response.address = request.address;
            response.fetch_id = request.fetch_id;
            response.epoch = request.epoch;
            const uint64_t index = request.address >= RESET_VECTOR ?
                (request.address - RESET_VECTOR) >> 2 : UINT64_MAX;
            response.instruction = index < words.size() ? words[(size_t)index] : 0x00000013;
            pipe.imem_resp.write(response);
        }
    }
};

struct PendingLoad {
    DmemRequest request;
    unsigned ready_cycle;
};

struct DataMemory {
    std::map<uint64_t, uint8_t> bytes;
    std::vector<PendingLoad> pending;
    bool saw_tohost;
    uint64_t tohost_value;
    unsigned loads;
    unsigned stores;
    unsigned load_delay;

    explicit DataMemory(bool branch_test)
        : saw_tohost(false), tohost_value(0), loads(0), stores(0),
          load_delay(branch_test ? 24 : 4) {
        bytes[UINT64_C(0x80000100)] = 1;
    }

    uint64_t read64(uint64_t address) const {
        uint64_t value = 0;
        for (unsigned i = 0; i < 8; ++i) {
            std::map<uint64_t, uint8_t>::const_iterator it = bytes.find(address + i);
            if (it != bytes.end()) value |= (uint64_t)it->second << (8 * i);
        }
        return value;
    }

    void respond(PipeSignals& pipe, unsigned cycle) {
        if (pending.empty() || pending.front().ready_cycle > cycle || pipe.dmem_resp.full()) return;
        DmemResponse response;
        response.transaction_id = pending.front().request.transaction_id;
        response.data = read64(pending.front().request.address);
        response.read_data = response.data;
        pipe.dmem_resp.write(response);
        pending.erase(pending.begin());
    }

    void accept(PipeSignals& pipe, unsigned cycle) {
        while (!pipe.dmem_req.empty()) {
            const DmemRequest request = pipe.dmem_req.read();
            if (!request.is_store) {
                PendingLoad load;
                load.request = request;
                load.ready_cycle = cycle + load_delay;
                pending.push_back(load);
                ++loads;
                continue;
            }
            ++stores;
            const uint8_t mask = request.write_mask ? request.write_mask : request.mask;
            for (unsigned i = 0; i < 8; ++i)
                if (mask & (1u << i)) bytes[request.address + i] =
                    (uint8_t)(request.write_data >> (8 * i));
            if (request.address == TOHOST) {
                saw_tohost = true;
                tohost_value = request.write_data;
            }
        }
    }
};

struct DividerRecord {
    uint64_t pc;
    uint8_t pdst;
    uint32_t allocation_id;
    unsigned accepted_cycle;
    unsigned busy_cycles;
    unsigned writeback_cycle;
    uint64_t writeback_value;
    uint64_t committed_value;
    bool wrote_back;
    bool committed;
};

static DividerRecord* find_record(std::vector<DividerRecord>& records,
                                  uint32_t allocation_id) {
    for (size_t i = 0; i < records.size(); ++i)
        if (records[i].allocation_id == allocation_id) return &records[i];
    return 0;
}

static bool run_test(const TestSpec& spec, unsigned& suite_max_latency) {
    std::vector<uint32_t> program;
    if (!load_program(spec, program)) return false;

    BoomCoreState state;
    PipeSignals pipe;
    InstructionMemory imem(program);
    DataMemory dmem(spec.branch_kill);
    std::vector<CommitEntry> commits;
    std::vector<DividerRecord> divider_records;
    bool previous_token = false;
    bool commit_tohost = false;
    uint64_t commit_tohost_value = 0;
    unsigned cycles = 0;

    for (; cycles < 3000 && !dmem.saw_tohost && !state.io_trap; ++cycles) {
        imem.step(pipe, cycles);
        dmem.respond(pipe, cycles);
        boom_core_step(state, pipe);

        const DividerExecutionState& divider = state.execute.divider;
        if (!previous_token && divider.token_valid) {
            DividerRecord record;
            record.pc = divider.uop.debug_pc;
            record.pdst = divider.pdst;
            record.allocation_id = divider.allocation_id;
            record.accepted_cycle = cycles;
            record.busy_cycles = 0;
            record.writeback_cycle = 0;
            record.writeback_value = 0;
            record.committed_value = 0;
            record.wrote_back = false;
            record.committed = false;
            divider_records.push_back(record);
        }
        if (divider.token_valid && divider.arithmetic.busy) {
            DividerRecord* record = find_record(divider_records, divider.allocation_id);
            if (record) ++record->busy_cycles;
        }
        for (unsigned port = 0; port < NUM_INT_WRITEBACK_PORTS; ++port) {
            const WritebackEvent& wb = state.completion.writebacks[port];
            if (!wb.valid) continue;
            DividerRecord* record = find_record(divider_records, wb.rob_allocation_id);
            if (record && wb.pdst == record->pdst) {
                record->wrote_back = true;
                record->writeback_cycle = cycles;
                record->writeback_value = wb.value;
            }
        }

        while (!pipe.commit_trace.empty()) {
            const CommitEntry entry = pipe.commit_trace.read();
            commits.push_back(entry);
            if (is_divider_instruction(entry.inst)) {
                for (size_t i = 0; i < divider_records.size(); ++i)
                    if (divider_records[i].pc == entry.pc && !divider_records[i].committed) {
                        divider_records[i].committed = true;
                        divider_records[i].committed_value = entry.rd_value;
                        break;
                    }
            }
            if (entry.is_store && entry.memory_address == TOHOST) {
                commit_tohost = true;
                commit_tohost_value = entry.memory_data;
            }
        }
        dmem.accept(pipe, cycles);
        previous_token = divider.token_valid;
    }

    if (state.io_trap || !dmem.saw_tohost || dmem.tohost_value != 1 ||
        !commit_tohost || commit_tohost_value != 1 || state.tohost != 1) {
        std::printf("FAIL %-22s tohost/trap failure (memory=%d/%llu commit=%d/%llu state=%llu cycles=%u commits=%u dividers=%u rob=%u/%u)\n",
                    spec.name, dmem.saw_tohost, (unsigned long long)dmem.tohost_value,
                    commit_tohost, (unsigned long long)commit_tohost_value,
                    (unsigned long long)state.tohost, cycles, (unsigned)commits.size(),
                    (unsigned)divider_records.size(), state.rob.head, state.rob.tail);
        const size_t first = commits.size() > 8 ? commits.size() - 8 : 0;
        const size_t initial = commits.size() < 24 ? commits.size() : 24;
        for (size_t i = 0; i < initial; ++i)
            std::printf("  initial pc=%016llx inst=%08x rd=%u value=%016llx store=%d addr=%016llx\n",
                        (unsigned long long)commits[i].pc, commits[i].inst, commits[i].rd,
                        (unsigned long long)commits[i].rd_value, commits[i].is_store,
                        (unsigned long long)commits[i].memory_address);
        for (size_t i = first; i < commits.size(); ++i)
            std::printf("  commit pc=%016llx inst=%08x rd=%u store=%d addr=%016llx\n",
                        (unsigned long long)commits[i].pc, commits[i].inst, commits[i].rd,
                        commits[i].is_store, (unsigned long long)commits[i].memory_address);
        return false;
    }

    for (size_t e = 0; e < spec.expected.size(); ++e) {
        bool found = false;
        for (size_t c = 0; c < commits.size(); ++c)
            if (commits[c].rd_valid && commits[c].rd == spec.expected[e].rd &&
                commits[c].rd_value == spec.expected[e].value) found = true;
        if (!found) {
            std::printf("FAIL %-22s x%u expected %016llx not committed\n", spec.name,
                        spec.expected[e].rd, (unsigned long long)spec.expected[e].value);
            return false;
        }
    }

    unsigned divider_commits = 0;
    unsigned divider_writebacks = 0;
    unsigned max_busy = 0;
    unsigned max_latency = 0;
    for (size_t i = 0; i < divider_records.size(); ++i) {
        const DividerRecord& record = divider_records[i];
        if (record.committed) ++divider_commits;
        if (record.wrote_back) ++divider_writebacks;
        if (record.busy_cycles > max_busy) max_busy = record.busy_cycles;
        if (record.wrote_back) {
            const unsigned latency = record.writeback_cycle - record.accepted_cycle;
            if (latency > max_latency) max_latency = latency;
        }
        if (record.committed && (!record.wrote_back ||
                                 record.writeback_value != record.committed_value)) {
            std::printf("FAIL %-22s divider at %016llx commit/writeback mismatch\n",
                        spec.name, (unsigned long long)record.pc);
            return false;
        }
    }
    if (divider_commits != spec.committed_dividers ||
        divider_writebacks < spec.committed_dividers || max_busy < spec.minimum_busy_cycles) {
        std::printf("FAIL %-22s divider metrics accept=%u wb=%u commit=%u busy=%u\n",
                    spec.name, (unsigned)divider_records.size(), divider_writebacks,
                    divider_commits, max_busy);
        return false;
    }
    if (spec.needs_load && dmem.loads == 0) {
        std::printf("FAIL %-22s did not issue a real data-memory load\n", spec.name);
        return false;
    }
    if (spec.branch_kill) {
        bool wrong_accepted = false;
        bool wrong_suppressed = false;
        for (size_t i = 0; i < divider_records.size(); ++i) {
            if (divider_records[i].pc == RESET_VECTOR + 24) {
                wrong_accepted = true;
                wrong_suppressed = !divider_records[i].wrote_back && !divider_records[i].committed &&
                                   divider_records[i].busy_cycles != 0;
            }
        }
        bool wrong_rd_committed = false;
        for (size_t i = 0; i < commits.size(); ++i)
            if (commits[i].rd_valid && (commits[i].rd == 20 || commits[i].rd == 21))
                wrong_rd_committed = true;
        if (!wrong_accepted || !wrong_suppressed || wrong_rd_committed) {
            std::printf("FAIL %-22s wrong-path divider was not accepted and killed cleanly\n", spec.name);
            return false;
        }
    }
    if (max_latency > suite_max_latency) suite_max_latency = max_latency;
    std::printf("PASS %-22s cycles=%u accept=%u wb=%u commit=%u max_latency=%u busy=%u\n",
                spec.name, cycles, (unsigned)divider_records.size(), divider_writebacks,
                divider_commits, max_latency, max_busy);
    return true;
}

int main() {
    const std::vector<TestSpec> tests = {
        {"div", {{3, UINT64_C(0xfffffffffffffff2)}, {4, UINT64_C(0xfffffffffffffff3)}}, 1, 1, 60, false, false},
        {"divu", {{3, UINT64_C(0x1999999999999999)}, {4, UINT64_C(0x199999999999999a)}}, 1, 1, 60, false, false},
        {"rem", {{3, UINT64_C(0xfffffffffffffffe)}, {4, UINT64_MAX}}, 1, 1, 60, false, false},
        {"remu", {{3, 5}, {4, 6}}, 1, 1, 60, false, false},
        {"word_div_rem_mix", {{3, UINT64_C(0xffffffffd5555556)}, {4, UINT64_C(0x2aaaaaaa)},
                              {5, UINT64_C(0xfffffffffffffffe)}, {6, 2}}, 4, 4, 30, false, false},
        {"divide_by_zero", {{3, UINT64_MAX}, {4, 123}, {5, UINT64_MAX}, {6, 123}}, 4, 4, 0, false, false},
        {"divide_overflow", {{3, UINT64_C(0x8000000000000000)}, {4, 0},
                             {6, UINT64_C(0xffffffff80000000)}, {7, 0}}, 4, 4, 0, false, false},
        {"divide_dependency", {{3, 14}, {5, 7}, {6, 2}, {7, 8}}, 3, 3, 60, false, false},
        {"divide_load_mix", {{2, 100}, {4, 11}, {5, 1}}, 2, 2, 60, true, false},
        {"divide_branch_kill", {{4, 14}, {5, 15}}, 2, 1, 60, true, true}
    };

    unsigned passed = 0;
    unsigned suite_max_latency = 0;
    for (size_t i = 0; i < tests.size(); ++i)
        if (run_test(tests[i], suite_max_latency)) ++passed;
    if (passed != tests.size() || suite_max_latency < 64) {
        std::printf("M3B native full-core divider programs: %u/%u PASS, max latency %u cycles\n",
                    passed, (unsigned)tests.size(), suite_max_latency);
        return 1;
    }
    std::printf("M3B native full-core divider programs: 10/10 PASS, commit/writeback checked, max latency %u cycles\n",
                suite_max_latency);
    return 0;
}
