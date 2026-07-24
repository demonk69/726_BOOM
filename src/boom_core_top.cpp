#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

void boom_core_top(hls::stream<ImemRequest>&  imem_req_out,
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
#pragma HLS RESET variable=state

    PipeSignals pipe;

CORE_CYCLE:
    while (true) {
#pragma HLS PIPELINE II=1

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
}
