#include "trace_reader.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace boom {

class TraceCompare {
public:
    TraceCompare() : m_mismatches(0), m_total(0) {}

    bool compare_entries(const TraceEntry& ref, const TraceEntry& dut,
                         uint64_t cycle, int slot) {
        m_total++;
        bool match = true;

        if (ref.pc != dut.pc) {
            printf("[MISMATCH] cycle=%lu slot=%d: PC ref=0x%lx dut=0x%lx\n",
                   cycle, slot, ref.pc, dut.pc);
            match = false;
        }
        if (ref.instruction != dut.instruction) {
            printf("[MISMATCH] cycle=%lu slot=%d: INSN ref=0x%x dut=0x%x\n",
                   cycle, slot, ref.instruction, dut.instruction);
            match = false;
        }
        if (ref.rd != dut.rd) {
            printf("[MISMATCH] cycle=%lu slot=%d: RD ref=%d dut=%d\n",
                   cycle, slot, ref.rd, dut.rd);
            match = false;
        }
        if (ref.rd_value != dut.rd_value) {
            printf("[MISMATCH] cycle=%lu slot=%d: RD_VAL ref=0x%lx dut=0x%lx\n",
                   cycle, slot, ref.rd_value, dut.rd_value);
            match = false;
        }
        if (ref.exception != dut.exception) {
            printf("[MISMATCH] cycle=%lu slot=%d: EXC ref=%d dut=%d\n",
                   cycle, slot, ref.exception, dut.exception);
            match = false;
        }
        if (ref.branch_mispredict != dut.branch_mispredict) {
            printf("[MISMATCH] cycle=%lu slot=%d: MISPRED ref=%d dut=%d\n",
                   cycle, slot, ref.branch_mispredict, dut.branch_mispredict);
            match = false;
        }

        if (!match) m_mismatches++;
        return match;
    }

    void report() const {
        printf("=== Trace Comparison Report ===\n");
        printf("  Total entries compared: %d\n", m_total);
        printf("  Mismatches:            %d\n", m_mismatches);
        if (m_total > 0) {
            printf("  Match rate:            %.2f%%\n",
                   100.0 * (m_total - m_mismatches) / m_total);
        }
        printf("  Result:                %s\n",
               m_mismatches == 0 ? "PASS" : "FAIL");
    }

    int get_mismatches() const { return m_mismatches; }
    int get_total() const { return m_total; }

private:
    int m_mismatches;
    int m_total;
};

}
