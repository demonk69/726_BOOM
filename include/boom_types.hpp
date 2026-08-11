#ifndef BOOM_TYPES_HPP
#define BOOM_TYPES_HPP

#include "boom_config.hpp"
#include <cstdint>

enum FuCode : uint8_t {
    FU_ALU  = 0, FU_MUL = 1, FU_DIV = 2, FU_CSR = 3,
    FU_FPU  = 4, FU_MEM = 5, FU_I2F = 6, FU_F2I = 7, FU_FDV = 8
};
enum IqType : uint8_t { IQ_MEM = 0, IQ_ALU = 1, IQ_FPU = 2 };
enum IssuePortClass : uint8_t {
    ISSUE_PORT_INT = 0, ISSUE_PORT_MEM = 1, ISSUE_PORT_UNSUPPORTED = 2
};
enum BrType : uint8_t { BR_N=0, BR_NE=1, BR_EQ=2, BR_GE=3, BR_GEU=4, BR_LT=5, BR_LTU=6, BR_J=7, BR_JR=8 };
enum Op1Sel : uint8_t { OP1_RS1=0, OP1_IMU=1, OP1_PC=2, OP1_X0=3 };
enum Op2Sel : uint8_t { OP2_RS2=0, OP2_IMM=1, OP2_IMU=2, OP2_IMZ=3, OP2_X0=4 };
enum ImmSel : uint8_t { IMM_I=0, IMM_S=1, IMM_B=2, IMM_U=3, IMM_J=4, IMM_Z=5 };
enum CsrCmd : uint8_t { CSR_N=0, CSR_R=1, CSR_W=2, CSR_S=3, CSR_C=4, CSR_P=5 };
enum DstRType : uint8_t { DST_INT=0, DST_FP=1, DST_X0=2, DST_N=3 };
enum RobState : uint8_t { ROB_INIT=0, ROB_NORMAL=1, ROB_FLUSH=2, ROB_EXCEPTION=3 };
enum PrivilegeMode : uint8_t { PRV_U=0, PRV_S=1, PRV_H=2, PRV_M=3 };
enum DmemCommand : uint8_t { DMEM_LOAD=0, DMEM_STORE=1 };

enum CompletionKind : uint8_t {
    COMPLETION_NONE = 0, COMPLETION_EXECUTE = 1,
    COMPLETION_MEMORY_ADDRESS = 2, COMPLETION_STORE = 3,
    COMPLETION_LOAD_RESPONSE = 4, COMPLETION_BRANCH = 5
};
enum CompletionSourceId : uint8_t {
    COMPLETION_SOURCE_LSU_LOAD = 0, COMPLETION_SOURCE_MEM_EXECUTE = 1,
    COMPLETION_SOURCE_INT_EXECUTE = 2, COMPLETION_SOURCE_COUNT = 3
};
enum RobCompletePortId : uint8_t {
    ROB_COMPLETE_PORT_LSU_LOAD = 0,
    ROB_COMPLETE_PORT_MEM_EXECUTE = 1,
    ROB_COMPLETE_PORT_INT_EXECUTE = 2,
    ROB_COMPLETE_PORT_UNSUPPORTED = 3
};

struct DecodeControl {
    uint8_t br_type, op1_sel, op2_sel, imm_sel, op_fcn, fcn_dw, csr_cmd;
    bool is_load, is_sta, is_std;
};
struct BranchInfo {
    uint8_t br_mask, br_tag, ftq_idx, pc_lob;
    bool edge_inst, taken, is_br, is_jalr, is_jal, is_sfb;
};
struct RenameInfo {
    uint8_t ldst, lrs1, lrs2, lrs3, pdst, prs1, prs2, prs3, ppred;
    bool prs1_busy, prs2_busy, prs3_busy, ppred_busy;
    uint8_t stale_pdst, dst_rtype, lrs1_rtype, lrs2_rtype;
    bool frs3_en, fp_val, fp_single;
};
struct QueueInfo {
    uint8_t rob_idx, ldq_idx, stq_idx, rxq_idx;
    uint32_t rob_allocation_id;
};
struct MemoryInfo {
    uint8_t mem_cmd, mem_size;
    bool mem_signed, is_fence, is_fencei, is_amo, uses_ldq, uses_stq, ldst_is_rs1, ldst_val;
};
struct ExceptionInfo {
    bool exception; uint64_t exc_cause;
    bool xcpt_pf_if, xcpt_ae_if, xcpt_ma_if, bp_debug_if, bp_xcpt_if;
};
struct DebugInfo { uint8_t debug_fsrc, debug_tsrc; uint32_t debug_inst; uint64_t debug_pc; };

struct MicroOp {
    uint8_t uopc; uint32_t inst, debug_inst; bool is_rvc; uint64_t debug_pc;
    uint8_t iq_type, fu_code;
    DecodeControl ctrl;
    uint8_t iw_state; bool iw_p1_poisoned, iw_p2_poisoned;
    BranchInfo branch;
    uint32_t imm_packed; uint16_t csr_addr;
    QueueInfo queue; RenameInfo rename;
    bool exception; uint64_t exc_cause;
    bool bypassable;
    MemoryInfo mem;
    bool is_sys_pc2epc, is_unique, flush_on_commit;
    ExceptionInfo exc; DebugInfo debug;
    MicroOp() { uopc=0; inst=0; debug_inst=0; is_rvc=false; debug_pc=0; iq_type=0; fu_code=0;
        ctrl={}; iw_state=0; iw_p1_poisoned=iw_p2_poisoned=false; branch={}; imm_packed=0; csr_addr=0;
        queue={}; rename={}; exception=false; exc_cause=0; bypassable=false; mem={};
        is_sys_pc2epc=is_unique=flush_on_commit=false; exc={}; debug={}; }
};

// Fixed-size completion payload. Arrays of this type are only accessed at
// constant port indices in W4B to avoid aggregate mux synthesis artifacts.
struct RobCompleteEvent {
    bool valid;
    CompletionKind kind;
    CompletionSourceId source;
    MicroOp uop;
    bool writes_prf, mispredict, control_resolved;
    uint64_t redirect_pc, value;
    bool exception;
    uint64_t exc_cause;
    bool memory_valid, is_load, is_store, signed_load;
    uint64_t memory_address, store_data;
    uint8_t memory_mask, memory_size;
    uint32_t transaction_id;
    RobCompleteEvent() : valid(false), kind(COMPLETION_NONE),
        source(COMPLETION_SOURCE_LSU_LOAD), uop(), writes_prf(false),
        mispredict(false), control_resolved(false), redirect_pc(0), value(0), exception(false),
        exc_cause(0), memory_valid(false), is_load(false), is_store(false),
        signed_load(false), memory_address(0), store_data(0), memory_mask(0),
        memory_size(0), transaction_id(0) {}
};

typedef RobCompleteEvent CompletionEvent;

struct WritebackEvent {
    bool valid;
    uint8_t pdst;
    uint64_t value;
    uint8_t rob_idx;
    uint32_t rob_allocation_id;
    CompletionSourceId source;
    WritebackEvent() : valid(false), pdst(0), value(0), rob_idx(0),
        rob_allocation_id(0), source(COMPLETION_SOURCE_LSU_LOAD) {}
};

struct WakeupEvent {
    bool valid;
    uint8_t pdst;
    uint64_t value;
    uint8_t rob_idx;
    uint32_t rob_allocation_id;
    uint8_t branch_mask;
    CompletionSourceId source;
    WakeupEvent() : valid(false), pdst(0), value(0), rob_idx(0),
        rob_allocation_id(0), branch_mask(0),
        source(COMPLETION_SOURCE_LSU_LOAD) {}
};

struct BypassEvent {
    bool valid;
    uint8_t pdst;
    uint64_t value;
    uint8_t rob_idx;
    uint32_t rob_allocation_id;
    uint8_t branch_mask;
    CompletionSourceId source;
    BypassEvent() : valid(false), pdst(0), value(0), rob_idx(0),
        rob_allocation_id(0), branch_mask(0),
        source(COMPLETION_SOURCE_LSU_LOAD) {}
};

struct IssueGrant {
    bool valid, accepted, from_dispatch;
    uint8_t entry_index, port_class;
    MicroOp uop;
    IssueGrant() : valid(false), accepted(false), from_dispatch(false),
        entry_index(0xff), port_class(ISSUE_PORT_UNSUPPORTED), uop() {}
};

struct RobEntry {
    bool valid, busy, unsafe, exception, exception_reported;
    MicroOp uop;
    bool memory_valid, is_load, is_store, signed_load;
    bool memory_request_sent, memory_completed;
    uint64_t memory_address, memory_data;
    uint8_t memory_mask, memory_size;
    uint32_t memory_transaction_id;
    RobEntry() : valid(false), busy(false), unsafe(false), exception(false),
        exception_reported(false), uop(),
        memory_valid(false), is_load(false), is_store(false), signed_load(false),
        memory_request_sent(false), memory_completed(false), memory_address(0),
        memory_data(0), memory_mask(0), memory_size(0), memory_transaction_id(0) {}
};

struct IssueSlotEntry {
    bool valid, request, granted, killed, pdst_busy, prs1_busy, prs2_busy, prs3_busy;
    MicroOp uop;
    uint64_t prs1_data, prs2_data, prs3_data;
    IssueSlotEntry() : valid(false), request(false), granted(false), killed(false),
        pdst_busy(false), prs1_busy(false), prs2_busy(false), prs3_busy(false),
        uop(), prs1_data(0), prs2_data(0), prs3_data(0) {}
};

struct BranchUpdate {
    uint8_t resolve_mask, mispredict_mask, cfi_type, pc_sel, br_tag;
    bool valid, mispredict, taken;
    uint64_t jalr_target; int64_t target_offset;
    MicroOp uop;
    BranchUpdate() : resolve_mask(0), mispredict_mask(0), cfi_type(0), pc_sel(0), br_tag(0),
        valid(false), mispredict(false), taken(false), jalr_target(0), target_offset(0), uop() {}
};

enum FrontendRedirectCause : uint8_t {
    FRONTEND_REDIRECT_DEBUG,
    FRONTEND_REDIRECT_EXCEPTION,
    FRONTEND_REDIRECT_INTERRUPT,
    FRONTEND_REDIRECT_ERET,
    FRONTEND_REDIRECT_FENCEI
};

struct FrontendRedirect {
    bool valid;
    uint64_t target_pc;
    FrontendRedirectCause cause;
    uint8_t rob_idx;
    uint32_t allocation_id;
    uint8_t branch_mask;
    FrontendRedirect() : valid(false), target_pc(0), cause(FRONTEND_REDIRECT_EXCEPTION),
        rob_idx(0), allocation_id(0), branch_mask(0) {}
};

struct CommitEntry {
    bool valid; uint64_t pc; uint32_t inst; uint8_t rd, priv; uint64_t rd_value;
    bool exception, rd_valid; uint64_t exc_cause;
    bool branch_mispredict;
    bool memory_valid, is_store;
    uint64_t memory_address, memory_data, store_addr, store_data;
    uint8_t memory_mask, store_mask;
    CommitEntry() : valid(false), pc(0), inst(0), rd(0), priv(0), rd_value(0),
        exception(false), rd_valid(false), exc_cause(0), branch_mispredict(false),
        memory_valid(false), is_store(false), memory_address(0), memory_data(0),
        store_addr(0), store_data(0), memory_mask(0), store_mask(0) {}
};

struct ImemRequest {
    uint64_t address;
    uint32_t fetch_id;
    uint32_t epoch;
    bool     kill;
    ImemRequest() : address(0), fetch_id(0), epoch(0), kill(false) {}
};

struct ImemResponse {
    uint64_t address;
    uint32_t fetch_id;
    uint32_t epoch;
    uint32_t instruction;
    bool     exception;
    uint64_t exc_cause;
    ImemResponse() : address(0), fetch_id(0), epoch(0), instruction(0), exception(false), exc_cause(0) {}
};

struct DmemRequest {
    uint32_t transaction_id;
    uint8_t rob_idx, command, size, mask, write_mask, branch_mask, epoch;
    bool is_store, signed_load, committed;
    uint64_t address, data, write_data;
    DmemRequest() : transaction_id(0), rob_idx(0), command(DMEM_LOAD), size(3),
        mask(0), write_mask(0), branch_mask(0), epoch(0), is_store(false),
        signed_load(false), committed(false), address(0), data(0), write_data(0) {}
};

struct DmemResponse {
    uint32_t transaction_id;
    uint64_t data, read_data;
    bool exception; uint64_t exc_cause, exception_cause;
    DmemResponse() : transaction_id(0), data(0), read_data(0), exception(false),
        exc_cause(0), exception_cause(0) {}
};

#endif
