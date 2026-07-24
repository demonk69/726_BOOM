#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  [TEST] %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static uint32_t make_itype(uint32_t imm, uint8_t rs1, uint8_t f3, uint8_t rd, uint8_t opc) {
    return ((imm&0xFFF)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|opc;
}
static uint32_t make_rtype(uint8_t f7, uint8_t rs2, uint8_t rs1, uint8_t f3, uint8_t rd, uint8_t opc) {
    return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|opc;
}
static uint32_t make_utype(uint32_t imm, uint8_t rd, uint8_t opc) {
    return (imm&0xFFFFF000)|(rd<<7)|opc;
}
static uint32_t make_jtype(uint32_t imm, uint8_t rd, uint8_t opc) {
    uint32_t v=((imm>>20)&0x1)<<31; v|=((imm>>1)&0x3FF)<<21; v|=((imm>>11)&0x1)<<20; v|=((imm>>12)&0xFF)<<12;
    return v|(rd<<7)|opc;
}
static uint32_t make_ecall() { return 0x00000073; }

struct IdealMem {
    static const int MAX_WORDS = 256;
    uint64_t base;
    uint32_t words[MAX_WORDS];
    int count;

    void clear(uint64_t b) { base=b; count=0; memset(words,0,sizeof(words)); }
    void add(uint32_t w) { if(count<MAX_WORDS) words[count++]=w; }
    uint32_t read(uint64_t addr) {
        if (addr < base) return 0;
        uint32_t idx = (uint32_t)((addr - base) >> 2);
        if (idx >= (uint32_t)count) return 0;
        return words[idx];
    }
};

void run_program(const char* name, IdealMem& mem, int max_cycles, int expected_x1, int expected_x2, int expected_x3) {
    TEST(name);
    BoomCoreState state;
    PipeSignals pipe;
    IdealMem* mem_ptr = &mem;
    int commits = 0;

    for (int c=0; c<max_cycles; c++) {
        if (!pipe.imem_req.empty()) {
            ImemRequest req = pipe.imem_req.read();
            ImemResponse resp;
            resp.address = req.address;
            resp.fetch_id = req.fetch_id;
            resp.instruction = mem_ptr->read(req.address);
            resp.exception = false;
            resp.exc_cause = 0;
            if (!pipe.imem_resp.full())
                pipe.imem_resp.write(resp);
        }

        boom_core_step(state, pipe);

        while (!pipe.commit_trace.empty()) {
            CommitEntry ce = pipe.commit_trace.read();
            commits++;
        }

        if (state.io_trap && !state.io_success) {
            FAIL("io_trap set without success"); return;
        }
        if (state.io_success) break;
    }

    CHECK(commits > 0, "no commits");
    {
        uint8_t p1 = state.rename.int_map_table.committed_map_table[1];
        uint8_t p2 = state.rename.int_map_table.committed_map_table[2];
        uint8_t p3 = state.rename.int_map_table.committed_map_table[3];
        uint64_t x1 = (p1>0) ? state.int_rf[p1] : 0;
        uint64_t x2 = (p2>0) ? state.int_rf[p2] : 0;
        uint64_t x3 = (p3>0) ? state.int_rf[p3] : 0;
        if (x1 != (uint64_t)expected_x1) { FAIL("x1 mismatch"); return; }
        if (x2 != (uint64_t)expected_x2) { FAIL("x2 mismatch"); return; }
        if (x3 != (uint64_t)expected_x3) { FAIL("x3 mismatch"); return; }
    }
    CHECK(state.csr.instret > 0, "instret unchanged");
    CHECK(state.csr.cycle > 0, "cycle unchanged");
    PASS();
}

void test_addi_add_ecall() {
    IdealMem mem; mem.clear(RESET_VECTOR);
    mem.add(make_itype(5, 0, 0, 1, 0x13));    // ADDI x1, x0, 5
    mem.add(make_itype(7, 1, 0, 2, 0x13));    // ADDI x2, x1, 7
    mem.add(make_rtype(0, 2, 1, 0, 3, 0x33)); // ADD x3, x1, x2
    mem.add(make_ecall());                       // ECALL
    run_program("ADDI+ADD+ECALL chain", mem, 200, 5, 12, 17);
}

void test_alu_chain() {
    IdealMem mem; mem.clear(RESET_VECTOR);
    mem.add(make_itype(3, 0, 0, 1, 0x13));     // ADDI x1, x0, 3
    mem.add(make_itype(2, 1, 0, 2, 0x13));     // ADDI x2, x1, 2  (RAW on x1)
    mem.add(make_rtype(0, 1, 2, 0, 3, 0x33));  // ADD x3, x2, x1  (RAW on x1,x2)
    mem.add(make_ecall());
    run_program("ALU RAW chain", mem, 200, 3, 5, 8);
}

void test_jump() {
    IdealMem mem; mem.clear(RESET_VECTOR);
    mem.add(make_jtype(8, 0, 0x6F));            // JAL x0, +8  (skip next)
    mem.add(make_itype(99, 0, 0, 1, 0x13));     // ADDI x1, x0, 99 (should be skipped)
    mem.add(make_itype(1, 0, 0, 1, 0x13));     // ADDI x1, x0, 1
    mem.add(make_ecall());
    run_program("JAL skip", mem, 200, 1, 0, 0);
}

void test_branch_beq() {
    IdealMem mem; mem.clear(RESET_VECTOR);
    mem.add(make_itype(5, 0, 0, 1, 0x13));     // ADDI x1, x0, 5
    mem.add(make_itype(5, 0, 0, 2, 0x13));     // ADDI x2, x0, 5
    // beq x1, x2, +8
    uint32_t beq_inst = 0;
    { int32_t imm=8; uint32_t b_imm=((imm>>12)&1)<<31; b_imm|=((imm>>5)&0x3F)<<25; b_imm|=((imm>>1)&0xF)<<8; b_imm|=((imm>>11)&1)<<7; beq_inst=b_imm|(2<<20)|(1<<15)|(0<<12)|0x63; }
    mem.add(beq_inst);                            // BEQ x1, x2, +8 (taken)
    mem.add(make_itype(99, 0, 0, 3, 0x13));     // ADDI x3, x0, 99 (skipped)
    mem.add(make_itype(10, 0, 0, 3, 0x13));     // ADDI x3, x0, 10
    mem.add(make_ecall());
    run_program("BEQ taken", mem, 200, 5, 5, 10);
}

void test_branch_not_taken() {
    IdealMem mem; mem.clear(RESET_VECTOR);
    mem.add(make_itype(3, 0, 0, 1, 0x13));
    mem.add(make_itype(7, 0, 0, 2, 0x13));
    uint32_t bne_inst = 0;
    { int32_t imm=8; uint32_t b_imm=((imm>>12)&1)<<31; b_imm|=((imm>>5)&0x3F)<<25; b_imm|=((imm>>1)&0xF)<<8; b_imm|=((imm>>11)&1)<<7; bne_inst=b_imm|(2<<20)|(1<<15)|(1<<12)|0x63; }
    mem.add(bne_inst);                            // BNE x1, x2, +8 (taken: 3!=7)
    mem.add(make_itype(99, 0, 0, 3, 0x13));     // skipped
    mem.add(make_itype(10, 0, 0, 3, 0x13));
    mem.add(make_ecall());
    run_program("BNE taken (3!=7)", mem, 200, 3, 7, 10);
}

int main() {
    printf("=== BOOM HLS Observable Pipeline Testbench ===\n\n");
    test_addi_add_ecall();
    test_alu_chain();
    test_jump();
    test_branch_beq();
    test_branch_not_taken();
    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return (tests_failed>0)?1:0;
}
