#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "reset.hpp"

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static void drive_reset_outputs(bool& io_success,
                                bool& io_halted,
                                bool& io_trap,
                                bool& io_cycle_valid,
                                uint64_t& io_cycle,
                                uint64_t& io_instret) {
    io_success = false;
    io_halted = false;
    io_trap = false;
    io_cycle_valid = false;
    io_cycle = 0;
    io_instret = 0;
}

static void boom_core_cycle_or_reset(BoomCoreState& state,
                                     ResetControllerState& reset_ctrl,
                                     PipeSignals& pipe,
                                     hls::stream<ImemRequest>& imem_req_out,
                                     hls::stream<ImemResponse>& imem_resp_in,
                                     hls::stream<DmemRequest>& dmem_req_out,
                                     hls::stream<DmemResponse>& dmem_resp_in,
                                     hls::stream<CommitEntry>& commit_trace_out,
                                     bool& io_success,
                                     bool& io_halted,
                                     bool& io_trap,
                                     bool& io_cycle_valid,
                                     uint64_t& io_cycle,
                                     uint64_t& io_instret);

static void boom_core_cycle_io(BoomCoreState& state,
                               PipeSignals& pipe,
                               hls::stream<ImemRequest>&  imem_req_out,
                               hls::stream<ImemResponse>& imem_resp_in,
                               hls::stream<DmemRequest>&  dmem_req_out,
                               hls::stream<DmemResponse>& dmem_resp_in,
                               hls::stream<CommitEntry>&  commit_trace_out,
                               bool& io_success,
                               bool& io_halted,
                               bool& io_trap,
                               bool& io_cycle_valid,
                               uint64_t& io_cycle,
                               uint64_t& io_instret) {
    if (!imem_resp_in.empty() && !pipe.imem_resp.full())
        pipe.imem_resp.write(imem_resp_in.read());
    if (!dmem_resp_in.empty() && !pipe.dmem_resp.full())
        pipe.dmem_resp.write(dmem_resp_in.read());

    boom_core_step(state, pipe);

    if (!pipe.imem_req.empty())
        imem_req_out.write(pipe.imem_req.read());
    if (!pipe.dmem_req.empty())
        dmem_req_out.write(pipe.dmem_req.read());
    if (!pipe.commit_trace.empty())
        commit_trace_out.write(pipe.commit_trace.read());

    io_success = state.io_success;
    io_halted = state.io_halted;
    io_trap = state.io_trap;
    io_cycle_valid = true;
    io_cycle = state.csr.cycle;
    io_instret = state.csr.instret;
}

static void boom_core_cycle_or_reset(BoomCoreState& state,
                                     ResetControllerState& reset_ctrl,
                                     PipeSignals& pipe,
                                     hls::stream<ImemRequest>& imem_req_out,
                                     hls::stream<ImemResponse>& imem_resp_in,
                                     hls::stream<DmemRequest>& dmem_req_out,
                                     hls::stream<DmemResponse>& dmem_resp_in,
                                     hls::stream<CommitEntry>& commit_trace_out,
                                     bool& io_success,
                                     bool& io_halted,
                                     bool& io_trap,
                                     bool& io_cycle_valid,
                                     uint64_t& io_cycle,
                                     uint64_t& io_instret) {
    if (!reset_ctrl.completed) {
        boom_core_reset_step(state, reset_ctrl);
        drive_reset_outputs(io_success, io_halted, io_trap,
                            io_cycle_valid, io_cycle, io_instret);
        return;
    }
    boom_core_cycle_io(state, pipe, imem_req_out, imem_resp_in,
                       dmem_req_out, dmem_resp_in, commit_trace_out,
                       io_success, io_halted, io_trap,
                       io_cycle_valid, io_cycle, io_instret);
}

void boom_core_top(hls::stream<ImemRequest>&  imem_req_out,
                   hls::stream<ImemResponse>& imem_resp_in,
                   hls::stream<DmemRequest>&  dmem_req_out,
                   hls::stream<DmemResponse>& dmem_resp_in,
                   hls::stream<CommitEntry>&  commit_trace_out,
                    volatile bool* io_success,
                    volatile bool* io_halted,
                    volatile bool* io_trap,
                    volatile bool* io_cycle_valid,
                    volatile uint64_t* io_cycle,
                    volatile uint64_t* io_instret) {
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=imem_req_out
#pragma HLS INTERFACE axis port=imem_resp_in
#pragma HLS INTERFACE axis port=dmem_req_out
#pragma HLS INTERFACE axis port=dmem_resp_in
#pragma HLS INTERFACE axis port=commit_trace_out
#pragma HLS INTERFACE ap_none port=io_success
#pragma HLS INTERFACE ap_none port=io_halted
#pragma HLS INTERFACE ap_none port=io_trap
#pragma HLS INTERFACE ap_none port=io_cycle_valid
#pragma HLS INTERFACE ap_none port=io_cycle
#pragma HLS INTERFACE ap_none port=io_instret

    static BoomCoreState state;
    static ResetControllerState reset_ctrl;
#pragma HLS RESET variable=reset_ctrl

    PipeSignals pipe;
    bool success, halted, trap, cycle_valid;
    uint64_t cycle, instret;

CORE_CYCLE:
    while (true) {
#ifdef BOOM_HLS_ENABLE_CORE_PIPELINE
#pragma HLS PIPELINE II=1
#endif
        boom_core_cycle_or_reset(state, reset_ctrl, pipe,
                                 imem_req_out, imem_resp_in,
                                 dmem_req_out, dmem_resp_in, commit_trace_out,
                                 success, halted, trap,
                                 cycle_valid, cycle, instret);
        *io_success = success;
        *io_halted = halted;
        *io_trap = trap;
        *io_cycle_valid = cycle_valid;
        *io_cycle = cycle;
        *io_instret = instret;
    }
}

void boom_core_step_top(hls::stream<ImemRequest>&  imem_req_out,
                        hls::stream<ImemResponse>& imem_resp_in,
                        hls::stream<DmemRequest>&  dmem_req_out,
                        hls::stream<DmemResponse>& dmem_resp_in,
                        hls::stream<CommitEntry>&  commit_trace_out,
                         volatile bool* io_success,
                         volatile bool* io_halted,
                         volatile bool* io_trap,
                         volatile bool* io_cycle_valid,
                         volatile uint64_t* io_cycle,
                         volatile uint64_t* io_instret) {
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=imem_req_out
#pragma HLS INTERFACE axis port=imem_resp_in
#pragma HLS INTERFACE axis port=dmem_req_out
#pragma HLS INTERFACE axis port=dmem_resp_in
#pragma HLS INTERFACE axis port=commit_trace_out
#pragma HLS INTERFACE ap_none port=io_success
#pragma HLS INTERFACE ap_none port=io_halted
#pragma HLS INTERFACE ap_none port=io_trap
#pragma HLS INTERFACE ap_none port=io_cycle_valid
#pragma HLS INTERFACE ap_none port=io_cycle
#pragma HLS INTERFACE ap_none port=io_instret

    static BoomCoreState state;
    static ResetControllerState reset_ctrl;
#pragma HLS RESET variable=reset_ctrl

    PipeSignals pipe;
    bool success, halted, trap, cycle_valid;
    uint64_t cycle, instret;
    boom_core_cycle_or_reset(state, reset_ctrl, pipe,
                             imem_req_out, imem_resp_in,
                             dmem_req_out, dmem_resp_in, commit_trace_out,
                             success, halted, trap,
                             cycle_valid, cycle, instret);
    *io_success = success;
    *io_halted = halted;
    *io_trap = trap;
    *io_cycle_valid = cycle_valid;
    *io_cycle = cycle;
    *io_instret = instret;
}

#define BOOM_NCYCLE_INTERFACES() \
    _Pragma("HLS INTERFACE ap_ctrl_none port=return") \
    _Pragma("HLS INTERFACE axis port=imem_req_out") \
    _Pragma("HLS INTERFACE axis port=imem_resp_in") \
    _Pragma("HLS INTERFACE axis port=dmem_req_out") \
    _Pragma("HLS INTERFACE axis port=dmem_resp_in") \
    _Pragma("HLS INTERFACE axis port=commit_trace_out") \
    _Pragma("HLS INTERFACE ap_none port=io_success") \
    _Pragma("HLS INTERFACE ap_none port=io_halted") \
    _Pragma("HLS INTERFACE ap_none port=io_trap") \
    _Pragma("HLS INTERFACE ap_none port=io_cycle_valid") \
    _Pragma("HLS INTERFACE ap_none port=io_cycle") \
    _Pragma("HLS INTERFACE ap_none port=io_instret")

#define DEFINE_BOOM_NCYCLE_TOP(NAME, CYCLES) \
void NAME(hls::stream<ImemRequest>& imem_req_out, \
          hls::stream<ImemResponse>& imem_resp_in, \
          hls::stream<DmemRequest>& dmem_req_out, \
          hls::stream<DmemResponse>& dmem_resp_in, \
          hls::stream<CommitEntry>& commit_trace_out, \
          volatile bool* io_success, volatile bool* io_halted, volatile bool* io_trap, \
          volatile bool* io_cycle_valid, volatile uint64_t* io_cycle, volatile uint64_t* io_instret) { \
    BOOM_NCYCLE_INTERFACES(); \
    static BoomCoreState state; \
    static ResetControllerState reset_ctrl; \
    _Pragma("HLS RESET variable=reset_ctrl") \
    PipeSignals pipe; \
    bool success, halted, trap, cycle_valid; \
    uint64_t cycle_value, instret_value; \
    for (int cycle = 0; cycle < CYCLES; cycle++) { \
        boom_core_cycle_or_reset(state, reset_ctrl, pipe, \
                                 imem_req_out, imem_resp_in, \
                                 dmem_req_out, dmem_resp_in, commit_trace_out, \
                                 success, halted, trap, \
                                 cycle_valid, cycle_value, instret_value); \
    } \
    *io_success = success; *io_halted = halted; *io_trap = trap; \
    *io_cycle_valid = cycle_valid; *io_cycle = cycle_value; \
    *io_instret = instret_value; \
}

DEFINE_BOOM_NCYCLE_TOP(boom_core_ncycle_n1_top, 1)
DEFINE_BOOM_NCYCLE_TOP(boom_core_ncycle_n2_top, 2)
DEFINE_BOOM_NCYCLE_TOP(boom_core_ncycle_n4_top, 4)
DEFINE_BOOM_NCYCLE_TOP(boom_core_ncycle_n8_top, 8)

#undef DEFINE_BOOM_NCYCLE_TOP
#undef BOOM_NCYCLE_INTERFACES
