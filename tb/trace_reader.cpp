#include "boom_config.hpp"
#include "boom_types.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace boom {

struct TraceEntry {
    uint64_t cycle;
    uint8_t  commit_slot;
    uint64_t pc;
    uint32_t instruction;
    uint8_t  privilege;
    bool     rd_valid;
    uint8_t  rd;
    uint64_t rd_value;
    bool     exception;
    uint64_t exception_cause;
    bool     memory_valid;
    uint64_t memory_address;
    uint64_t memory_data;
    uint8_t  memory_mask;
    bool     branch_mispredict;

    TraceEntry() : cycle(0), commit_slot(0), pc(0), instruction(0),
        privilege(0), rd_valid(false), rd(0), rd_value(0),
        exception(false), exception_cause(0), memory_valid(false),
        memory_address(0), memory_data(0), memory_mask(0),
        branch_mispredict(false) {}
};

class TraceReader {
public:
    TraceReader() : m_file(nullptr), m_good(false) {}

    bool open(const char* filename) {
        m_file = fopen(filename, "r");
        m_good = (m_file != nullptr);
        return m_good;
    }

    void close() {
        if (m_file) { fclose(m_file); m_file = nullptr; }
        m_good = false;
    }

    bool good() const { return m_good; }

    bool read_entry(TraceEntry& entry) {
        if (!m_good) return false;
        int n = fscanf(m_file, "%lu,%hhu,%lx,%x,%hhu,%d,%hhu,%lx,%d,%lx,%d,%lx,%lx,%hhu,%d\n",
            &entry.cycle, &entry.commit_slot, &entry.pc, &entry.instruction,
            &entry.privilege, (int*)&entry.rd_valid, &entry.rd,
            &entry.rd_value, (int*)&entry.exception, &entry.exception_cause,
            (int*)&entry.memory_valid, &entry.memory_address, &entry.memory_data,
            &entry.memory_mask, (int*)&entry.branch_mispredict);
        return n == 15;
    }

    static TraceEntry from_commit_entry(const CommitEntry& ce, uint64_t cycle, int slot) {
        TraceEntry e;
        e.cycle = cycle;
        e.commit_slot = slot;
        e.pc = ce.pc;
        e.instruction = ce.inst;
        e.privilege = ce.priv;
        e.rd_valid = ce.rd_valid;
        e.rd = ce.rd;
        e.rd_value = ce.rd_value;
        e.exception = ce.exception;
        e.exception_cause = ce.exc_cause;
        e.memory_valid = ce.is_store;
        e.memory_address = ce.store_addr;
        e.memory_data = ce.store_data;
        e.memory_mask = ce.store_mask;
        e.branch_mispredict = ce.branch_mispredict;
        return e;
    }

private:
    FILE* m_file;
    bool  m_good;
};

}
