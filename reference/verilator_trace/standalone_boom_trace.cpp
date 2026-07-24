// Standalone BOOM Verilator trace runner for generated VTestHarness artifacts.
// This avoids FESVR/DRAMSim by providing local DPI stubs and a simple AXI memory.

#include "VTestHarness.h"
#include "verilated.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define CORE_SIG(name) \
    (top->TestHarness__DOT__chiptop__DOT__system__DOT__tile_prci_domain__DOT__tile_reset_domain__DOT__boom_tile__DOT__core__DOT__##name)
#define FTQ_SIG(name) \
    (top->TestHarness__DOT__chiptop__DOT__system__DOT__tile_prci_domain__DOT__tile_reset_domain__DOT__boom_tile__DOT__frontend__DOT__ftq__DOT__##name)
#define SYSTEM_SIG(name) \
    (top->TestHarness__DOT__chiptop__DOT__system__DOT__##name)

static uint64_t g_cycle = 0;
static uint64_t g_trace_sequence_id = 0;
static uint64_t g_commit_records = 0;
static uint64_t g_last_recorded_cycle = std::numeric_limits<uint64_t>::max();
static uint64_t g_last_commit_pc = 0;
static bool g_last_commit_pc_valid = false;
static uint64_t g_arch_regs[32] = {0};
static bool g_tohost_seen = false;
static uint64_t g_tohost_value = 0;
static const char* g_tohost_source = "none";
static std::string g_trace_path;
static std::string g_loadmem_path;
static uint64_t g_loadmem_addr = 0;
static uint64_t g_tohost_addr = UINT64_C(0x80000080);
static uint64_t g_max_cycles = 100000;
static bool g_wake_hart0 = false;
static bool g_disable_assert_stops = false;
static bool g_after_reset = false;

bool verbose = false;
bool done_reset = false;

double sc_time_stamp() { return static_cast<double>(g_cycle); }

static std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static uint64_t parse_u64(const std::string& value, int base = 0) {
    char* end = nullptr;
    uint64_t parsed = std::strtoull(value.c_str(), &end, base);
    if (!end || *end != '\0') {
        std::cerr << "invalid integer: " << value << "\n";
        std::exit(2);
    }
    return parsed;
}

static std::string hex_u64(uint64_t value) {
    std::ostringstream os;
    os << "0x" << std::hex << std::setfill('0') << std::setw(16) << value;
    return os.str();
}

static std::string hex_u32(uint32_t value) {
    std::ostringstream os;
    os << "0x" << std::hex << std::setfill('0') << std::setw(8) << value;
    return os.str();
}

static std::string json_escape(const std::string& value) {
    std::ostringstream os;
    for (char c : value) {
        if (c == '"' || c == '\\') os << '\\' << c;
        else if (c == '\n') os << "\\n";
        else os << c;
    }
    return os.str();
}

static void json_hex_or_null(std::ofstream& out, const char* key, bool valid, uint64_t value, bool word32 = false) {
    out << ",\"" << key << "\":";
    if (!valid) {
        out << "null";
    } else if (word32) {
        out << "\"" << hex_u32(static_cast<uint32_t>(value)) << "\"";
    } else {
        out << "\"" << hex_u64(value) << "\"";
    }
}

struct ReadResp {
    uint32_t id;
    uint64_t data;
    bool last;
};

class SimpleMemory {
  public:
    explicit SimpleMemory(uint64_t size, int word_size, int line_size)
        : size_(size), word_size_(word_size), line_size_(line_size) {
        if (word_size_ <= 0 || line_size_ <= 0) {
            std::cerr << "invalid memory geometry\n";
            std::exit(2);
        }
    }

    void load_hex(uint64_t start, const std::string& path) {
        std::ifstream in(path.c_str());
        if (!in) {
            std::cerr << "could not open loadmem file: " << path << "\n";
            std::exit(3);
        }
        std::string line;
        uint64_t addr = start;
        while (std::getline(in, line)) {
            line = trim(line);
            const size_t comment = line.find('#');
            if (comment != std::string::npos) line = trim(line.substr(0, comment));
            if (line.empty()) continue;
            if (line.size() & 1U) {
                std::cerr << "odd-length loadmem line in " << path << ": " << line << "\n";
                std::exit(3);
            }
            uint64_t j = 0;
            for (int64_t i = static_cast<int64_t>(line.size()) - 2; i >= 0; i -= 2) {
                const int hi = hex_nibble(line[static_cast<size_t>(i)]);
                const int lo = hex_nibble(line[static_cast<size_t>(i + 1)]);
                if (hi < 0 || lo < 0) {
                    std::cerr << "non-hex loadmem line in " << path << ": " << line << "\n";
                    std::exit(3);
                }
                bytes_[wrap(addr + j)] = static_cast<uint8_t>((hi << 4) | lo);
                j++;
            }
            addr += static_cast<uint64_t>(line.size() / 2);
        }
    }

    bool read_u32(uint64_t addr, uint32_t* value) const {
        uint32_t result = 0;
        bool seen = false;
        for (int i = 0; i < 4; ++i) {
            const auto it = bytes_.find(wrap(addr + static_cast<uint64_t>(i)));
            if (it != bytes_.end()) {
                result |= static_cast<uint32_t>(it->second) << (8 * i);
                seen = true;
            }
        }
        *value = result;
        return seen;
    }

    void watch_tohost(uint64_t addr) {
        tohost_addr_ = addr;
        tohost_enabled_ = addr != 0;
        tohost_seen_ = false;
        tohost_value_ = 0;
    }

    bool tohost_seen() const { return tohost_seen_; }
    uint64_t tohost_value() const { return tohost_value_; }

    void tick(bool reset,
              bool ar_valid, uint64_t ar_addr, uint32_t ar_id, uint32_t ar_size, uint32_t ar_len,
              bool aw_valid, uint64_t aw_addr, uint32_t aw_id, uint32_t aw_size, uint32_t aw_len,
              bool w_valid, uint64_t w_strb, uint64_t w_data, bool w_last,
              bool r_ready, bool b_ready) {
        if (reset) {
            rresp_.clear();
            bresp_.clear();
            store_inflight_ = false;
            tohost_seen_ = false;
            tohost_value_ = 0;
            return;
        }

        const bool ar_fire = ar_valid && ar_ready();
        const bool aw_fire = aw_valid && aw_ready();
        const bool w_fire = w_valid && w_ready();
        const bool r_fire = r_valid() && r_ready;
        const bool b_fire = b_valid() && b_ready;

        if (ar_fire) {
            const uint64_t start = (ar_addr / static_cast<uint64_t>(word_size_)) * static_cast<uint64_t>(word_size_);
            for (uint32_t beat = 0; beat <= ar_len; ++beat) {
                rresp_.push_back(ReadResp{ar_id, read_word(start + beat * static_cast<uint64_t>(word_size_)), beat == ar_len});
            }
        }

        if (aw_fire) {
            store_addr_ = aw_addr;
            store_id_ = aw_id;
            store_count_ = static_cast<uint64_t>(aw_len) + 1U;
            store_size_ = UINT64_C(1) << std::min<uint32_t>(aw_size, 3U);
            store_inflight_ = true;
        }

        if (w_fire) {
            write_word(store_addr_, w_data, w_strb, store_size_);
            store_addr_ += store_size_;
            if (store_count_ > 0) store_count_--;
            if (store_count_ == 0) {
                store_inflight_ = false;
                bresp_.push_back(store_id_);
                if (!w_last) std::cerr << "warning: AXI write burst ended without last\n";
            }
        }

        if (r_fire) rresp_.pop_front();
        if (b_fire) bresp_.pop_front();
    }

    bool ar_ready() const { return true; }
    bool aw_ready() const { return !store_inflight_; }
    bool w_ready() const { return store_inflight_; }
    bool r_valid() const { return !rresp_.empty(); }
    bool b_valid() const { return !bresp_.empty(); }
    uint32_t r_id() const { return rresp_.empty() ? 0 : rresp_.front().id; }
    uint32_t b_id() const { return bresp_.empty() ? 0 : bresp_.front(); }
    uint64_t r_data() const { return rresp_.empty() ? 0 : rresp_.front().data; }
    bool r_last() const { return !rresp_.empty() && rresp_.front().last; }

  private:
    uint64_t wrap(uint64_t addr) const { return size_ == 0 ? addr : (addr % size_); }

    uint64_t read_word(uint64_t addr) const {
        uint64_t result = 0;
        for (int i = 0; i < word_size_; ++i) {
            const auto it = bytes_.find(wrap(addr + static_cast<uint64_t>(i)));
            const uint8_t byte = it == bytes_.end() ? 0 : it->second;
            if (i < 8) result |= static_cast<uint64_t>(byte) << (8 * i);
        }
        return result;
    }

    uint64_t read_double(uint64_t addr) const {
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            const auto it = bytes_.find(wrap(addr + static_cast<uint64_t>(i)));
            const uint8_t byte = it == bytes_.end() ? 0 : it->second;
            result |= static_cast<uint64_t>(byte) << (8 * i);
        }
        return result;
    }

    void update_tohost(uint64_t base) {
        if (!tohost_enabled_) return;
        const uint64_t end = base + static_cast<uint64_t>(word_size_);
        if (tohost_addr_ >= base && tohost_addr_ < end) {
            const uint64_t value = read_double(tohost_addr_);
            if (value != 0) {
                tohost_seen_ = true;
                tohost_value_ = value;
            }
        }
    }

    void write_word(uint64_t addr, uint64_t data, uint64_t strb, uint64_t size) {
        strb &= ((UINT64_C(1) << size) - 1U) << (addr % static_cast<uint64_t>(word_size_));
        const uint64_t base = (addr / static_cast<uint64_t>(word_size_)) * static_cast<uint64_t>(word_size_);
        for (int i = 0; i < word_size_ && i < 8; ++i) {
            if (strb & (UINT64_C(1) << i)) bytes_[wrap(base + static_cast<uint64_t>(i))] = static_cast<uint8_t>((data >> (8 * i)) & 0xffU);
        }
        update_tohost(base);
    }

    uint64_t size_;
    int word_size_;
    int line_size_;
    bool store_inflight_ = false;
    uint64_t store_addr_ = 0;
    uint32_t store_id_ = 0;
    uint64_t store_size_ = 0;
    uint64_t store_count_ = 0;
    std::deque<ReadResp> rresp_;
    std::deque<uint32_t> bresp_;
    std::unordered_map<uint64_t, uint8_t> bytes_;
    bool tohost_enabled_ = false;
    bool tohost_seen_ = false;
    uint64_t tohost_addr_ = 0;
    uint64_t tohost_value_ = 0;
};

static SimpleMemory* g_memory = nullptr;

extern "C" void* memory_init(long long mem_size, long long word_size, long long line_size, long long, long long) {
    g_memory = new SimpleMemory(static_cast<uint64_t>(mem_size), static_cast<int>(word_size), static_cast<int>(line_size));
    if (!g_loadmem_path.empty()) g_memory->load_hex(g_loadmem_addr, g_loadmem_path);
    g_memory->watch_tohost(g_tohost_addr);
    return g_memory;
}

extern "C" void memory_tick(void* channel,
                            unsigned char reset,
                            unsigned char ar_valid, unsigned char* ar_ready, int ar_addr, int ar_id, int ar_size, int ar_len,
                            unsigned char aw_valid, unsigned char* aw_ready, int aw_addr, int aw_id, int aw_size, int aw_len,
                            unsigned char w_valid, unsigned char* w_ready, int w_strb, long long w_data, unsigned char w_last,
                            unsigned char* r_valid, unsigned char r_ready, int* r_id, int* r_resp, long long* r_data, unsigned char* r_last,
                            unsigned char* b_valid, unsigned char b_ready, int* b_id, int* b_resp) {
    SimpleMemory* mem = static_cast<SimpleMemory*>(channel);
    if (!mem) {
        *ar_ready = *aw_ready = *w_ready = *r_valid = *r_last = *b_valid = 0;
        *r_id = *r_resp = *b_id = *b_resp = 0;
        *r_data = 0;
        return;
    }
    mem->tick(reset,
              ar_valid, static_cast<uint32_t>(ar_addr), static_cast<uint32_t>(ar_id), static_cast<uint32_t>(ar_size), static_cast<uint32_t>(ar_len),
              aw_valid, static_cast<uint32_t>(aw_addr), static_cast<uint32_t>(aw_id), static_cast<uint32_t>(aw_size), static_cast<uint32_t>(aw_len),
              w_valid, static_cast<uint64_t>(w_strb), static_cast<uint64_t>(w_data), w_last,
              r_ready, b_ready);
    *ar_ready = mem->ar_ready();
    *aw_ready = mem->aw_ready();
    *w_ready = mem->w_ready();
    *r_valid = mem->r_valid();
    *r_id = static_cast<int>(mem->r_id());
    *r_resp = 0;
    *r_data = static_cast<long long>(mem->r_data());
    *r_last = mem->r_last();
    *b_valid = mem->b_valid();
    *b_id = static_cast<int>(mem->b_id());
    *b_resp = 0;
}

extern "C" int serial_tick(unsigned char, unsigned char* out_ready, int,
                            unsigned char* in_valid, unsigned char, int* in_bits) {
    *out_ready = 1;
    *in_valid = 0;
    *in_bits = 0;
    return 0;
}

extern "C" int jtag_tick(unsigned char* jtag_TCK, unsigned char* jtag_TMS, unsigned char* jtag_TDI,
                          unsigned char* jtag_TRSTn, unsigned char) {
    *jtag_TCK = 0;
    *jtag_TMS = 0;
    *jtag_TDI = 0;
    *jtag_TRSTn = 1;
    return 0;
}

extern "C" void uart_init(const char*, int) {}

extern "C" void uart_tick(unsigned char, unsigned char* serial_out_ready, char,
                           unsigned char* serial_in_valid, unsigned char, char* serial_in_bits) {
    *serial_out_ready = 1;
    *serial_in_valid = 0;
    *serial_in_bits = 0;
}

static uint64_t ftq_pc(VTestHarness* top, uint32_t idx) {
    switch (idx & 15U) {
        case 0: return FTQ_SIG(pcs_0);
        case 1: return FTQ_SIG(pcs_1);
        case 2: return FTQ_SIG(pcs_2);
        case 3: return FTQ_SIG(pcs_3);
        case 4: return FTQ_SIG(pcs_4);
        case 5: return FTQ_SIG(pcs_5);
        case 6: return FTQ_SIG(pcs_6);
        case 7: return FTQ_SIG(pcs_7);
        case 8: return FTQ_SIG(pcs_8);
        case 9: return FTQ_SIG(pcs_9);
        case 10: return FTQ_SIG(pcs_10);
        case 11: return FTQ_SIG(pcs_11);
        case 12: return FTQ_SIG(pcs_12);
        case 13: return FTQ_SIG(pcs_13);
        case 14: return FTQ_SIG(pcs_14);
        default: return FTQ_SIG(pcs_15);
    }
}

static uint64_t uop_pc(VTestHarness* top, uint32_t ftq_idx, uint32_t pc_lob) {
    return (ftq_pc(top, ftq_idx) & ~UINT64_C(0x3f)) | static_cast<uint64_t>(pc_lob & 0x3fU);
}

static int64_t sign_extend(uint32_t value, unsigned bits) {
    const uint32_t sign_bit = UINT32_C(1) << (bits - 1U);
    return static_cast<int64_t>((value ^ sign_bit) - sign_bit);
}

static bool memory_observed_tohost() {
    return g_memory && g_memory->tohost_seen();
}

static bool observed_tohost() {
    return g_tohost_seen || memory_observed_tohost();
}

static uint64_t observed_tohost_value() {
    if (g_tohost_seen) return g_tohost_value;
    return memory_observed_tohost() ? g_memory->tohost_value() : 0;
}

static const char* observed_tohost_source() {
    if (g_tohost_seen) return g_tohost_source;
    return memory_observed_tohost() ? "memory_write" : "none";
}

static bool detect_tohost_store(uint32_t inst, uint64_t* address, uint64_t* value) {
    const uint32_t opcode = inst & 0x7fU;
    const uint32_t funct3 = (inst >> 12) & 0x7U;
    if (opcode != 0x23U || funct3 != 0x3U) return false;
    const uint32_t rs1 = (inst >> 15) & 0x1fU;
    const uint32_t rs2 = (inst >> 20) & 0x1fU;
    const uint32_t imm = ((inst >> 7) & 0x1fU) | (((inst >> 25) & 0x7fU) << 5);
    const int64_t simm = sign_extend(imm, 12);
    *address = static_cast<uint64_t>(static_cast<int64_t>(g_arch_regs[rs1]) + simm);
    *value = g_arch_regs[rs2];
    return *address == g_tohost_addr && *value != 0;
}

static void trace_cycle(VTestHarness* top, std::ofstream& out) {
    if (!g_after_reset || g_last_recorded_cycle == g_cycle) return;
    g_last_recorded_cycle = g_cycle;

    if (CORE_SIG(rob_io_enq_valids_0)) {
        out << "{\"cycle\":" << g_cycle
            << ",\"event\":\"rob_allocate\",\"slot\":0"
            << ",\"rob_idx\":" << static_cast<unsigned>(CORE_SIG(rob_io_enq_uops_0_rob_idx))
            << ",\"physical_rd\":" << static_cast<unsigned>(CORE_SIG(rob_io_enq_uops_0_pdst))
            << ",\"stale_physical_rd\":" << static_cast<unsigned>(CORE_SIG(rob_io_enq_uops_0_stale_pdst))
            << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
    }

    if (CORE_SIG(ll_wbarb_io_out_valid)) {
        out << "{\"cycle\":" << g_cycle
            << ",\"event\":\"writeback\",\"slot\":0"
            << ",\"rob_idx\":" << static_cast<unsigned>(CORE_SIG(ll_wbarb_io_out_bits_uop_rob_idx))
            << ",\"physical_rd\":" << static_cast<unsigned>(CORE_SIG(ll_wbarb_io_out_bits_uop_pdst));
        json_hex_or_null(out, "result", true, CORE_SIG(ll_wbarb_io_out_bits_data));
        out << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
    }

    if (CORE_SIG(brinfos_0_valid)) {
        const uint64_t pc = uop_pc(top, CORE_SIG(brinfos_0_uop_ftq_idx), CORE_SIG(brinfos_0_uop_pc_lob));
        out << "{\"cycle\":" << g_cycle
            << ",\"event\":\"branch_resolve\",\"slot\":0"
            << ",\"rob_idx\":" << static_cast<unsigned>(CORE_SIG(brinfos_0_uop_rob_idx));
        json_hex_or_null(out, "pc", true, pc);
        out << ",\"taken\":" << (CORE_SIG(brinfos_0_taken) ? "true" : "false")
            << ",\"branch_mispredict\":" << (CORE_SIG(brinfos_0_mispredict) ? "true" : "false")
            << ",\"branch_tag\":" << static_cast<unsigned>(CORE_SIG(brinfos_0_uop_br_tag))
            << ",\"branch_mask\":" << static_cast<unsigned>(CORE_SIG(brinfos_0_uop_br_mask))
            << ",\"cfi_type\":" << static_cast<unsigned>(CORE_SIG(brinfos_0_cfi_type))
            << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
    }

    if (CORE_SIG(rob_io_flush_valid)) {
        out << "{\"cycle\":" << g_cycle << ",\"event\":\"flush\"";
        json_hex_or_null(out, "target", true, CORE_SIG(io_ifu_redirect_pc_REG));
        out << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
    }

    if (CORE_SIG(rob_io_commit_valids_0)) {
        const uint32_t ftq_idx = CORE_SIG(rob_io_commit_uops_0_ftq_idx);
        const uint32_t pc_lob = CORE_SIG(rob_io_commit_uops_0_pc_lob);
        const uint64_t pc = uop_pc(top, ftq_idx, pc_lob);
        g_last_commit_pc = pc;
        g_last_commit_pc_valid = true;
        g_commit_records++;
        uint32_t inst = 0;
        const bool inst_valid = g_memory && g_memory->read_u32(pc, &inst);
        const uint32_t pdst = CORE_SIG(rob_io_commit_uops_0_pdst);
        const bool rd_valid = CORE_SIG(rob_io_commit_uops_0_ldst_val);
        const bool rd_value_valid = rd_valid && pdst < 52U;
        const uint64_t rd_value = rd_value_valid ? CORE_SIG(iregfile__DOT__regfile)[pdst] : 0;
        uint64_t tohost_store_addr = 0;
        uint64_t tohost_store_value = 0;
        const bool tohost_store = inst_valid && detect_tohost_store(inst, &tohost_store_addr, &tohost_store_value);
        if (tohost_store) {
            g_tohost_seen = true;
            g_tohost_value = tohost_store_value;
            g_tohost_source = "retired_store";
        }
        out << "{\"cycle\":" << g_cycle
            << ",\"event\":\"commit\",\"slot\":0";
        json_hex_or_null(out, "pc", true, pc);
        json_hex_or_null(out, "instruction", inst_valid, inst, true);
        out << ",\"rd_valid\":" << (rd_valid ? "true" : "false")
            << ",\"rd\":";
        if (rd_valid) out << static_cast<unsigned>(CORE_SIG(rob_io_commit_uops_0_ldst)); else out << "null";
        out << ",\"physical_rd\":" << static_cast<unsigned>(pdst)
            << ",\"stale_physical_rd\":" << static_cast<unsigned>(CORE_SIG(rob_io_commit_uops_0_stale_pdst));
        json_hex_or_null(out, "rd_value", rd_value_valid, rd_value);
        out << ",\"exception\":" << (CORE_SIG(rob_io_com_xcpt_valid) ? "true" : "false")
            << ",\"exception_cause\":null"
            << ",\"privilege\":null"
            << ",\"arch_valid\":" << (CORE_SIG(rob_io_commit_arch_valids_0) ? "true" : "false")
            << ",\"pc_source\":\"ftq_pc_plus_pc_lob\""
            << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
        if (tohost_store) {
            out << "{\"cycle\":" << g_cycle
                << ",\"event\":\"tohost\"";
            json_hex_or_null(out, "pc", true, pc);
            json_hex_or_null(out, "address", true, tohost_store_addr);
            json_hex_or_null(out, "value", true, tohost_store_value);
            out << ",\"source\":\"retired_store\""
                << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
        }
        if (rd_valid && rd_value_valid) {
            const uint32_t rd = CORE_SIG(rob_io_commit_uops_0_ldst);
            if (rd != 0 && rd < 32U) g_arch_regs[rd] = rd_value;
            g_arch_regs[0] = 0;
        }
    }
}

static void trace_metadata(std::ofstream& out, const char* phase, const char* termination_reason, int exit_code) {
    out << "{\"cycle\":" << g_cycle
        << ",\"event\":\"metadata\""
        << ",\"phase\":\"" << phase << "\""
        << ",\"runner\":\"standalone_boom_trace\""
        << ",\"trace_schema_version\":1"
        << ",\"build\":\"standalone_v2_tohost\""
        << ",\"loadmem\":\"" << json_escape(g_loadmem_path) << "\"";
    json_hex_or_null(out, "loadmem_addr", true, g_loadmem_addr);
    json_hex_or_null(out, "tohost_addr", true, g_tohost_addr);
    out << ",\"max_cycles\":" << g_max_cycles
        << ",\"commit_records\":" << g_commit_records
        << ",\"termination_reason\":\"" << termination_reason << "\""
        << ",\"exit_code\":" << exit_code;
    json_hex_or_null(out, "tohost_value", observed_tohost(), observed_tohost_value());
    out << ",\"tohost_source\":\"" << observed_tohost_source() << "\"";
    json_hex_or_null(out, "last_pc", g_last_commit_pc_valid, g_last_commit_pc);
    out << ",\"max_cycles_reached\":" << (g_cycle >= g_max_cycles ? "true" : "false")
        << ",\"trap_seen\":false"
        << ",\"trace_sequence_id\":" << g_trace_sequence_id++ << "}\n";
}

static void apply_standalone_injections(VTestHarness* top) {
    if (g_wake_hart0 && g_after_reset) SYSTEM_SIG(clint__DOT__ipi_0) = 1;
}

static void half_cycle(VTestHarness* top, int clock) {
    apply_standalone_injections(top);
    top->clock = clock;
    top->eval();
    apply_standalone_injections(top);
}

static void full_cycle(VTestHarness* top, std::ofstream* trace) {
    half_cycle(top, 0);
    half_cycle(top, 1);
    if (trace) trace_cycle(top, *trace);
    g_cycle++;
}

static void parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg.find("+boom_equiv_trace=") == 0) {
            g_trace_path = arg.substr(std::strlen("+boom_equiv_trace="));
        } else if (arg.find("+loadmem=") == 0) {
            g_loadmem_path = arg.substr(std::strlen("+loadmem="));
        } else if (arg.find("+loadmem_addr=") == 0) {
            g_loadmem_addr = parse_u64(arg.substr(std::strlen("+loadmem_addr=")), 16);
        } else if (arg.find("+tohost_addr=") == 0) {
            g_tohost_addr = parse_u64(arg.substr(std::strlen("+tohost_addr=")), 16);
        } else if (arg.find("+max-cycles=") == 0) {
            g_max_cycles = parse_u64(arg.substr(std::strlen("+max-cycles=")), 0);
        } else if (arg.find("+standalone_wake_hart0=") == 0) {
            g_wake_hart0 = parse_u64(arg.substr(std::strlen("+standalone_wake_hart0=")), 0) != 0;
        } else if (arg.find("+standalone_disable_assert_stops=") == 0) {
            g_disable_assert_stops = parse_u64(arg.substr(std::strlen("+standalone_disable_assert_stops=")), 0) != 0;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: standalone_boom_trace +boom_equiv_trace=FILE +loadmem=HEX +loadmem_addr=80000000 +tohost_addr=80000080 +max-cycles=N [+standalone_wake_hart0=1] [+standalone_disable_assert_stops=1]\n";
            std::exit(0);
        }
    }
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    if (g_trace_path.empty()) {
        std::cerr << "+boom_equiv_trace=<path> is required\n";
        return 2;
    }
    if (g_loadmem_path.empty()) {
        std::cerr << "+loadmem=<hex image> is required for standalone mode\n";
        return 2;
    }

    Verilated::commandArgs(argc, argv);
    VTestHarness* top = new VTestHarness;
    std::ofstream trace(g_trace_path.c_str());
    if (!trace) {
        std::cerr << "could not open trace output: " << g_trace_path << "\n";
        delete top;
        return 3;
    }

    top->reset = 0;
    top->clock = 0;
    top->eval();
    done_reset = false;
    g_after_reset = false;
    top->reset = 1;
    for (int i = 0; i < 100; ++i) full_cycle(top, nullptr);
    top->reset = 0;
    g_after_reset = true;
    done_reset = !g_disable_assert_stops;
    trace_metadata(trace, "start", "running", 0);

    while (!Verilated::gotFinish() && g_cycle < g_max_cycles && !observed_tohost()) {
        full_cycle(top, &trace);
    }

    const bool got_finish = Verilated::gotFinish();
    const bool got_tohost = observed_tohost();
    const bool max_cycles_reached = g_cycle >= g_max_cycles && !got_finish && !got_tohost;
    const char* termination_reason = got_tohost ? "tohost" : (got_finish ? "verilator_finish" : "max_cycles");
    int exit_code = 0;
    if (got_tohost) {
        const uint64_t value = observed_tohost_value();
        exit_code = value == 1 ? 0 : static_cast<int>((value >> 1) & UINT64_C(0x7fffffff));
        if (exit_code == 0 && value != 1) exit_code = 1;
    } else if (max_cycles_reached) {
        exit_code = 4;
    }
    trace_metadata(trace, "end", termination_reason, exit_code);

    trace.close();
    std::cerr << "standalone_trace end cycle=" << g_cycle
              << " got_finish=" << (got_finish ? 1 : 0)
              << " got_tohost=" << (got_tohost ? 1 : 0)
              << " io_success=" << static_cast<int>(top->io_success)
              << " records=" << g_trace_sequence_id
              << " commits=" << g_commit_records
              << " termination_reason=" << termination_reason
              << " exit_code=" << exit_code
              << " wake_hart0=" << (g_wake_hart0 ? 1 : 0)
              << " disable_assert_stops=" << (g_disable_assert_stops ? 1 : 0) << "\n";
    delete top;
    delete g_memory;
    g_memory = nullptr;
    return exit_code;
}
