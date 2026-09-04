#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "reset.hpp"
#include <cstdint>
#include <iostream>
#include <string>

void boom_core_step(BoomCoreState&, PipeSignals&);

struct ProgramCase {
    const char* name;
    uint32_t fault_word;
    bool fetch_fault;
    uint64_t expected_pc;
    uint64_t expected_cause;
};

static uint32_t instruction_at(const ProgramCase& tc, uint64_t address) {
    if (address == RESET_VECTOR) return 0x00100093u; // addi x1,x0,1
    if (address == RESET_VECTOR + 4) return tc.fault_word;
    if (address == RESET_VECTOR + 8) return 0x00200113u; // younger addi x2,x0,2
    if (address == BOOM_TRAP_VECTOR) return 0x05a00193u; // handler signature x3=0x5a
    if (address == BOOM_TRAP_VECTOR + 4) return 0x00000513u; // a0=0
    if (address == BOOM_TRAP_VECTOR + 8) return 0x00000073u; // host ECALL success
    return 0x00000013u;
}

static bool run_case(const ProgramCase& tc) {
    BoomCoreState s;
    PipeSignals p;
    ResetControllerState reset;
    for (int i = 0; i < 512 && !reset.completed; i++) boom_core_reset_step(s, reset);
    if (!reset.completed) return false;

    bool saw_exception = false;
    bool saw_handler_signature = false;
    bool wrong_younger_commit = false;
    uint64_t epc = 0;
    uint64_t cause = 0;
    for (int cycle = 0; cycle < 2000 && !s.io_success; cycle++) {
        while (!p.imem_req.empty()) {
            ImemRequest req = p.imem_req.read();
            ImemResponse resp;
            resp.address = req.address;
            resp.fetch_id = req.fetch_id;
            resp.epoch = req.epoch;
            resp.instruction = instruction_at(tc, req.address);
            if (tc.fetch_fault && req.address == RESET_VECTOR + 4) {
                resp.exception = true;
                resp.exc_cause = tc.expected_cause;
            }
            p.imem_resp.write(resp);
        }
        while (!p.dmem_req.empty()) p.dmem_req.read();
        boom_core_step(s, p);
        if (s.exception_commit.valid) {
            saw_exception = true;
            epc = s.exception_commit.pc;
            cause = s.exception_commit.cause;
        }
        while (!p.commit_trace.empty()) {
            CommitEntry ce = p.commit_trace.read();
            if (!ce.exception && ce.pc == BOOM_TRAP_VECTOR && ce.rd == 3 &&
                ce.rd_value == 0x5a) saw_handler_signature = true;
            if (!ce.exception && ce.pc == RESET_VECTOR + 8) wrong_younger_commit = true;
        }
    }
    bool pass = s.io_success && saw_exception && saw_handler_signature &&
        !wrong_younger_commit && epc == tc.expected_pc && cause == tc.expected_cause;
    std::cout << "PROGRAM " << tc.name << " status=" << (pass ? "PASS" : "FAIL")
              << " success=" << s.io_success << " exception=" << saw_exception
              << " handler=" << saw_handler_signature << " epc=0x" << std::hex << epc
              << " cause=0x" << cause << std::dec << "\n";
    return pass;
}

int main() {
    const ProgramCase cases[] = {
        {"illegal_trap", 0xffffffffu, false, RESET_VECTOR + 4, 2},
        {"ebreak_trap", 0x00100073u, false, RESET_VECTOR + 4, 3},
        {"compressed_ebreak_trap", 0x00019002u, false, RESET_VECTOR + 4, 3},
        {"fetch_fault_trap", 0x00000013u, true, RESET_VECTOR + 4, 1},
        {"exception_kills_younger", 0xffffffffu, false, RESET_VECTOR + 4, 2},
        {"exception_with_branch", 0x00100073u, false, RESET_VECTOR + 4, 3},
        {"exception_with_rv64m_pending", 0xffffffffu, false, RESET_VECTOR + 4, 2},
        {"exception_after_rvc_stream", 0x00019002u, false, RESET_VECTOR + 4, 3},
    };
    int failures = 0;
    for (const ProgramCase& tc : cases) failures += !run_case(tc);
    std::cout << "PF1_PROGRAMS cases=8 failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
