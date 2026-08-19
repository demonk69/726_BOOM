#ifndef BOOM_CONFIG_HPP
#define BOOM_CONFIG_HPP

#include <cstdint>

#define BOOM_SMALL_CONFIG 1

#define ISA_RV64IMAFDC 1
#define MMU_SV39 1

#define FETCH_WIDTH         4
#define FETCH_PACKET_WIDTH  2
#define DECODE_WIDTH        1
#define DISPATCH_WIDTH      1
#define ISSUE_WIDTH         3
#define EXECUTE_RESULT_LANES ISSUE_WIDTH
#define MEM_ISSUE_LANE      0
#define INT_ISSUE_LANE      1
#define FP_ISSUE_LANE       2
#define INTEGER_ISSUE_PORTS 2
#define COMMIT_WIDTH        1
#define MACHINE_WIDTH       1
#define NUM_ROB_COMPLETE_PORTS 4
#define COMPLETION_PENDING_SLOTS 3

#define ROB_DEPTH           32
#define ROB_IDX_BITS        5

#define INT_PHYS_REGS       52
#define FP_PHYS_REGS        48
#define PHYS_REG_BITS        6
#define LOGICAL_REG_COUNT   32

#define ISSUE_QUEUE_MEM_DEPTH   8
#define ISSUE_QUEUE_ALU_DEPTH   8
#define ISSUE_QUEUE_FPU_DEPTH   8
#define ISSUE_QUEUE_IDX_BITS    3

#define LDQ_DEPTH           8
#define STQ_DEPTH           8
#define LDQ_IDX_BITS        3
#define STQ_IDX_BITS        3

#define MAX_BRANCH_COUNT    8
#define BR_MASK_BITS        8
#define BR_TAG_BITS         3

#define FTQ_DEPTH           16
#define FTQ_IDX_BITS        4

#ifndef FETCH_BUFFER_DEPTH
#define FETCH_BUFFER_DEPTH  8
#endif

#define ICACHE_SETS         64
#define ICACHE_WAYS         4
#define ICACHE_BLOCK_BYTES  64
#define ICACHE_FETCH_BYTES  8
#define ICACHE_SIZE_BYTES   16384

#define DCACHE_SETS         64
#define DCACHE_WAYS         4
#define DCACHE_BLOCK_BYTES  64
#define DCACHE_SIZE_BYTES   16384
#define DCACHE_NMSHRS       2

#define L2_CACHE_SIZE_BYTES 524288
#define L2_CACHE_WAYS       8
#define L2_CACHE_BLOCK_BYTES 64
#define L2_CACHE_SETS       1024

#define PADDR_BITS          32
#define VADDR_BITS          39

#define RESET_VECTOR        0x10040ull

#define CLOCK_FREQ_HZ       100000000
#define CLOCK_PERIOD_NS     10

#define WRITEBACK_VALIDATION_FAULT_CAUSE 0x100ULL

#define MAIN_MEM_BASE       0x80000000ull
#define MAIN_MEM_SIZE       0x10000000ull

#define CLINT_BASE          0x02000000ull
#define CLINT_SIZE          0x00010000ull
#define PLIC_BASE           0x0C000000ull
#define PLIC_SIZE           0x04000000ull
#define UART_BASE           0x54000000ull
#define DEBUG_BASE          0x00000000ull
#define BOOT_ROM_BASE       0x00010000ull

#define NUM_INT_WAKEUP_PORTS 3
#define NUM_INT_BYPASS_PORTS 3
#define INUM_WAKEUP_PORTS   NUM_INT_WAKEUP_PORTS
#define INUM_BYPASS_PORTS   NUM_INT_BYPASS_PORTS
#define FP_WAKEUP_PORTS     2
#define FP_BYPASS_PORTS     0

#define INT_RF_READ_PORTS   4
#define INT_RF_WRITE_PORTS  2
#define NUM_INT_WRITEBACK_PORTS INT_RF_WRITE_PORTS
#define FP_RF_READ_PORTS    3
#define FP_RF_WRITE_PORTS   2

#define BRANCH_PREDICTOR_TYPE 1
#define BPD_TOTAL_SIZE_KB   14

#define HAS_FPU             1
#define HAS_FDIVSQRT        1
#define HAS_VM              1
#define HAS_RVC             1

static_assert(ROB_IDX_BITS >= 5, "ROB_IDX_BITS too small for ROB_DEPTH");
static_assert((1u << ROB_IDX_BITS) >= ROB_DEPTH, "ROB_IDX_BITS insufficient for ROB_DEPTH");
static_assert(PHYS_REG_BITS >= 6, "PHYS_REG_BITS too small");
static_assert((1u << PHYS_REG_BITS) >= INT_PHYS_REGS, "PHYS_REG_BITS insufficient for INT_PHYS_REGS");
static_assert((1u << PHYS_REG_BITS) >= FP_PHYS_REGS, "PHYS_REG_BITS insufficient for FP_PHYS_REGS");
static_assert(BR_TAG_BITS >= 3, "BR_TAG_BITS too small");
static_assert((1u << BR_TAG_BITS) >= MAX_BRANCH_COUNT, "BR_TAG_BITS insufficient for MAX_BRANCH_COUNT");
static_assert(LDQ_IDX_BITS >= 3, "LDQ_IDX_BITS too small");
static_assert((1u << LDQ_IDX_BITS) >= LDQ_DEPTH, "LDQ_IDX_BITS insufficient for LDQ_DEPTH");
static_assert(STQ_IDX_BITS >= 3, "STQ_IDX_BITS too small");
static_assert((1u << STQ_IDX_BITS) >= STQ_DEPTH, "STQ_IDX_BITS insufficient for STQ_DEPTH");
static_assert(FTQ_IDX_BITS >= 4, "FTQ_IDX_BITS too small");
static_assert((1u << FTQ_IDX_BITS) >= FTQ_DEPTH, "FTQ_IDX_BITS insufficient for FTQ_DEPTH");
static_assert(FETCH_BUFFER_DEPTH == 2 || FETCH_BUFFER_DEPTH == 4 ||
              FETCH_BUFFER_DEPTH == 8 || FETCH_BUFFER_DEPTH == 16,
              "FETCH_BUFFER_DEPTH must be 2, 4, 8, or 16");
static_assert(EXECUTE_RESULT_LANES == ISSUE_WIDTH, "execute result interface must match issue lanes");
static_assert(ISSUE_WIDTH == 3, "SmallBoom fixed issue interface must contain MEM, INT, and FP lanes");
static_assert(MEM_ISSUE_LANE != INT_ISSUE_LANE, "MEM and INT issue lanes must be distinct");
static_assert(FP_ISSUE_LANE < ISSUE_WIDTH, "FP reserved lane must exist");
static_assert(NUM_ROB_COMPLETE_PORTS == 4, "SmallBoom has four general ROB response ports");
static_assert(COMPLETION_PENDING_SLOTS == 3, "W4B retains two execute lanes and one load response");
static_assert(NUM_INT_WAKEUP_PORTS == 3, "SmallBoom has three regular integer wakeups");
static_assert(NUM_INT_BYPASS_PORTS == 3, "SmallBoom has three integer bypass buses");
static_assert(NUM_INT_WRITEBACK_PORTS == 2, "SmallBoom has exactly two integer PRF writes");
static_assert(COMMIT_WIDTH == 1, "W4D does not widen commit");

#endif
