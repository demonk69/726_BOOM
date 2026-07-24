#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

namespace {

struct ProgramSpec {
    const char* name;
    const char* hex_name;
    int prefix_commits;
    uint64_t unsupported_pc;
    uint32_t unsupported_inst;
};

const ProgramSpec kPrograms[] = {
    {"independent_alu", "independent_alu.hex", 8, 0x80000020ull, 0x0062b023u},
    {"raw_chain", "raw_chain.hex", 8, 0x80000020ull, 0x0062b023u},
    {"branch_taken", "branch_taken.hex", 9, 0x80000028ull, 0x0062b023u},
    {"branch_not_taken", "branch_not_taken.hex", 10, 0x80000028ull, 0x0062b023u},
    {"nested_branch", "nested_branch.hex", 10, 0x80000030ull, 0x0062b023u},
};

const uint64_t kEntryPc = 0x80000000ull;
const uint64_t kTohostAddr = 0x80000080ull;
const int kMaxCycles = 500;

bool complete_mode() {
    const char* mode = std::getenv("HLS_TRACE_MODE");
    return mode && std::string(mode) == "complete";
}

struct IdealMem {
    std::vector<uint32_t> words;
    std::vector<uint8_t> bytes;

    void append_word(uint32_t word) {
        words.push_back(word);
        bytes.push_back(static_cast<uint8_t>(word & 0xffu));
        bytes.push_back(static_cast<uint8_t>((word >> 8) & 0xffu));
        bytes.push_back(static_cast<uint8_t>((word >> 16) & 0xffu));
        bytes.push_back(static_cast<uint8_t>((word >> 24) & 0xffu));
    }

    bool load_hex(const std::string& path) {
        std::ifstream in(path.c_str());
        if (!in.good()) return false;

        std::string line;
        while (std::getline(in, line)) {
            std::size_t comment = line.find('#');
            if (comment != std::string::npos) line = line.substr(0, comment);
            std::string token;
            for (std::size_t i = 0; i < line.size(); ++i) {
                if (!std::isspace(static_cast<unsigned char>(line[i]))) token.push_back(line[i]);
            }
            if (token.empty()) continue;
            unsigned long long value = std::strtoull(token.c_str(), 0, 16);
            if (token.size() <= 8) {
                append_word(static_cast<uint32_t>(value & 0xffffffffull));
            } else {
                append_word(static_cast<uint32_t>(value & 0xffffffffull));
                append_word(static_cast<uint32_t>((value >> 32) & 0xffffffffull));
            }
        }
        return !words.empty();
    }

    uint32_t read(uint64_t addr) const {
        if (addr < kEntryPc) return 0;
        uint64_t index = (addr - kEntryPc) >> 2;
        if (index >= words.size()) return 0;
        return words[static_cast<std::size_t>(index)];
    }

    uint8_t read_byte(uint64_t addr) const {
        if (addr < kEntryPc) return 0;
        uint64_t offset = addr - kEntryPc;
        if (offset >= bytes.size()) return 0;
        return bytes[static_cast<std::size_t>(offset)];
    }

    uint64_t read_beat(uint64_t addr) const {
        uint64_t base = addr & ~0x7ull;
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(read_byte(base + static_cast<uint64_t>(i))) << (8 * i);
        }
        return value;
    }

    void write_byte(uint64_t addr, uint8_t value) {
        if (addr < kEntryPc) return;
        uint64_t offset = addr - kEntryPc;
        if (offset >= bytes.size()) bytes.resize(static_cast<std::size_t>(offset + 1), 0);
        bytes[static_cast<std::size_t>(offset)] = value;
    }

    void write(uint64_t addr, uint64_t data, uint8_t size) {
        unsigned bytes_to_write = (size >= 3) ? 8u : (1u << size);
        for (unsigned i = 0; i < bytes_to_write; ++i) {
            write_byte(addr + i, static_cast<uint8_t>((data >> (8 * i)) & 0xffull));
        }
    }
};

std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a[a.size() - 1] == '/') return a + b;
    return a + "/" + b;
}

const char* bool_text(bool value) { return value ? "true" : "false"; }

template <typename T>
void drain_stream(hls::stream<T>& stream) {
    while (!stream.empty()) (void)stream.read();
}

const char* mnemonic(uint32_t inst) {
    uint32_t opcode = inst & 0x7fu;
    uint32_t funct3 = (inst >> 12) & 0x7u;
    uint32_t funct7 = (inst >> 25) & 0x7fu;
    if (opcode == 0x13 && funct3 == 0) return "ADDI";
    if (opcode == 0x17) return "AUIPC";
    if (opcode == 0x33 && funct3 == 0 && funct7 == 0) return "ADD";
    if (opcode == 0x63 && funct3 == 0) return "BEQ";
    if (opcode == 0x63 && funct3 == 1) return "BNE";
    if (opcode == 0x23 && funct3 == 3) return "SD";
    if (opcode == 0x6f) return "JAL";
    if (opcode == 0x67) return "JALR";
    return "UNKNOWN";
}

bool is_control_inst(uint32_t inst) {
    uint32_t opcode = inst & 0x7fu;
    return opcode == 0x63u || opcode == 0x6fu || opcode == 0x67u;
}

void print_rd(FILE* out, const CommitEntry& ce) {
    if (ce.rd_valid) std::fprintf(out, "\"rd\":%u,\"rd_value\":\"0x%016llx\"", ce.rd, (unsigned long long)ce.rd_value);
    else std::fprintf(out, "\"rd\":null,\"rd_value\":null");
}

void emit_metadata(FILE* out, const char* phase, const char* source, const ProgramSpec& program,
                   int commits, const char* status, int cycle, unsigned long long seq) {
    bool complete = complete_mode();
    std::fprintf(out,
        "{\"cycle\":%d,\"event\":\"metadata\",\"phase\":\"%s\",\"runner\":\"hls_prefix_trace_tb\","
        "\"trace_schema_version\":1,\"source\":\"%s\",\"program\":\"%s\","
        "\"scope\":\"%s\","
        "\"entry_pc\":\"0x%016llx\",\"tohost_addr\":\"0x%016llx\","
        "\"prefix_commit_target\":%d,\"prefix_commit_records\":%d,\"status\":\"%s\","
        "\"unavailable_fields_are_null\":true,\"trace_sequence_id\":%llu}\n",
        cycle, phase, source, program.name,
        complete ? "complete_program_tohost" : "loaded_program_prefix_before_unsupported_tohost_store",
        (unsigned long long)kEntryPc,
        (unsigned long long)kTohostAddr, program.prefix_commits, commits, status, seq);
}

bool run_program(const ProgramSpec& program, const std::string& root, const std::string& out_dir, const char* source) {
    bool complete = complete_mode();
    IdealMem mem;
    std::string hex_path = join_path(join_path(root, "tb/programs/boom_reference/build"), program.hex_name);
    if (!mem.load_hex(hex_path)) {
        std::fprintf(stderr, "%s: failed to load %s\n", program.name, hex_path.c_str());
        return false;
    }

    std::string out_path = join_path(out_dir, std::string(program.name) + "_" + source + (complete ? "_full" : "") + ".jsonl");
    FILE* out = std::fopen(out_path.c_str(), "w");
    if (!out) {
        std::fprintf(stderr, "%s: failed to open %s\n", program.name, out_path.c_str());
        return false;
    }

    BoomCoreState state;
    PipeSignals pipe;
    state.frontend.pc = kEntryPc;
    state.frontend.reset_done = true;
    state.frontend.request_sent = false;
    state.frontend.response_received = false;
    state.frontend.fetch_packet_valid = false;

    unsigned long long seq = 0;
    int prefix_commits = 0;
    int target_commits = program.prefix_commits + (complete ? 1 : 0);
    bool saw_tohost = false;
    bool ok = false;
    emit_metadata(out, "start", source, program, 0, "running", 0, seq++);

    for (int c = 0; c < kMaxCycles && prefix_commits < target_commits && !saw_tohost; ++c) {
        if (!pipe.imem_req.empty()) {
            ImemRequest req = pipe.imem_req.read();
            ImemResponse resp;
            resp.address = req.address;
            resp.fetch_id = req.fetch_id;
            resp.instruction = mem.read(req.address);
            resp.exception = false;
            resp.exc_cause = 0;
            if (!pipe.imem_resp.full()) pipe.imem_resp.write(resp);
        }

        boom_core_step(state, pipe);

        for (int lane = 0; lane < DISPATCH_WIDTH; ++lane) {
            const ExecuteState::AluResult& r = state.execute.alu_results[lane];
            if (!r.valid || !is_control_inst(r.uop.inst) || r.uop.debug_pc < kEntryPc) continue;
            std::fprintf(out,
                "{\"cycle\":%llu,\"event\":\"branch\",\"source\":\"%s\",\"program\":\"%s\","
                "\"slot\":%d,\"pc\":\"0x%016llx\",\"instruction\":\"0x%08x\",\"mnemonic\":\"%s\","
                "\"taken\":%s,\"branch_mispredict\":%s,\"target\":\"0x%016llx\","
                "\"trace_sequence_id\":%llu}\n",
                (unsigned long long)state.csr.cycle, source, program.name, lane,
                (unsigned long long)r.uop.debug_pc, r.uop.inst, mnemonic(r.uop.inst),
                bool_text(r.mispredict), bool_text(r.mispredict), (unsigned long long)r.redirect_pc, seq++);
        }

        while (!pipe.commit_trace.empty() && prefix_commits < target_commits) {
            CommitEntry ce = pipe.commit_trace.read();
            if (ce.pc < kEntryPc) continue;
            std::fprintf(out,
                "{\"cycle\":%llu,\"event\":\"commit\",\"source\":\"%s\",\"program\":\"%s\","
                "\"slot\":0,\"pc\":\"0x%016llx\",\"instruction\":\"0x%08x\",\"mnemonic\":\"%s\","
                "\"rd_valid\":%s,",
                (unsigned long long)state.csr.cycle, source, program.name,
                (unsigned long long)ce.pc, ce.inst, mnemonic(ce.inst), bool_text(ce.rd_valid));
            print_rd(out, ce);
            std::fprintf(out,
                ",\"physical_rd\":null,\"stale_physical_rd\":null,\"privilege\":%u,"
                "\"exception\":%s,\"exception_cause\":%s,\"branch_mispredict\":null,",
                ce.priv, bool_text(ce.exception), ce.exception ? "0" : "null");
            if (ce.memory_valid) {
                std::fprintf(out,
                    "\"memory_valid\":true,\"memory_address\":\"0x%016llx\",\"memory_data\":\"0x%016llx\",\"memory_mask\":\"0x%02x\",\"is_store\":%s,",
                    (unsigned long long)ce.memory_address, (unsigned long long)ce.memory_data,
                    ce.memory_mask, bool_text(ce.is_store));
            } else {
                std::fprintf(out,
                    "\"memory_valid\":false,\"memory_address\":null,\"memory_data\":null,\"memory_mask\":null,\"is_store\":false,");
            }
            std::fprintf(out, "\"arch_valid\":true,\"trace_sequence_id\":%llu}\n", seq++);
            prefix_commits++;
        }

        while (!pipe.dmem_req.empty()) {
            DmemRequest req = pipe.dmem_req.read();
            if (req.is_store || req.command == DMEM_STORE) {
                uint64_t store_data = req.write_data ? req.write_data : req.data;
                uint8_t store_mask = req.write_mask ? req.write_mask : req.mask;
                mem.write(req.address, store_data, req.size);
                if (req.address == kTohostAddr) {
                    std::fprintf(out,
                        "{\"cycle\":%llu,\"event\":\"tohost\",\"source\":\"%s\",\"program\":\"%s\","
                        "\"address\":\"0x%016llx\",\"value\":\"0x%016llx\",\"mask\":\"0x%02x\","
                        "\"command\":\"store\",\"committed\":%s,\"trace_sequence_id\":%llu}\n",
                        (unsigned long long)state.csr.cycle, source, program.name,
                        (unsigned long long)req.address, (unsigned long long)store_data,
                        static_cast<unsigned>(store_mask), bool_text(req.committed), seq++);
                    saw_tohost = (store_data == 1);
                }
            } else {
                DmemResponse resp;
                resp.transaction_id = req.transaction_id;
                resp.data = mem.read_beat(req.address);
                resp.read_data = resp.data;
                resp.exception = false;
                resp.exc_cause = 0;
                resp.exception_cause = 0;
                if (!pipe.dmem_resp.full()) pipe.dmem_resp.write(resp);
            }
        }
    }

    if (!complete && prefix_commits == program.prefix_commits) {
        ok = true;
        std::fprintf(out,
            "{\"cycle\":%llu,\"event\":\"unsupported_boundary\",\"source\":\"%s\",\"program\":\"%s\","
            "\"pc\":\"0x%016llx\",\"instruction\":\"0x%08x\",\"mnemonic\":\"%s\","
            "\"reason\":\"prefix trace mode excludes the retired store-to-tohost boundary\","
            "\"trace_sequence_id\":%llu}\n",
            (unsigned long long)state.csr.cycle, source, program.name,
            (unsigned long long)program.unsupported_pc, program.unsupported_inst,
            mnemonic(program.unsupported_inst), seq++);
    }

    if (complete && saw_tohost && prefix_commits == target_commits) ok = true;

    emit_metadata(out, "end", source, program, prefix_commits, ok ? (complete ? "complete_tohost" : "prefix_complete") : (complete ? "complete_incomplete" : "prefix_incomplete"),
                  (int)state.csr.cycle, seq++);
    drain_stream(pipe.imem_req);
    drain_stream(pipe.imem_resp);
    drain_stream(pipe.dmem_req);
    drain_stream(pipe.dmem_resp);
    drain_stream(pipe.commit_trace);
    std::fclose(out);
    std::printf("%s: %s commits=%d target=%d trace=%s\n", program.name,
                ok ? "PASS" : "FAIL", prefix_commits, target_commits, out_path.c_str());
    return ok;
}

} // namespace

int main() {
    const char* root_env = std::getenv("HLS_PROJECT_ROOT");
    const char* out_env = std::getenv("HLS_TRACE_OUT_DIR");
    const char* source_env = std::getenv("HLS_TRACE_SOURCE");
    std::string root = root_env ? root_env : ".";
    std::string out_dir = out_env ? out_env : "reference/hls_traces";
    const char* source = source_env ? source_env : "hls_cpp";

    bool all_ok = true;
    for (std::size_t i = 0; i < sizeof(kPrograms) / sizeof(kPrograms[0]); ++i) {
        all_ok = run_program(kPrograms[i], root, out_dir, source) && all_ok;
    }
    return all_ok ? 0 : 1;
}
