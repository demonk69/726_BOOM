# Gate 5.1 Focused RTL Baseline

- Git HEAD: `c6e1d0401e55cf1cdb90b8af8fe00c14ea8449f1` (`Gate 5.1: implement frontend request and redirect foundation`).
- Branch/upstream: `gate3.8-rtl-verification`, `origin/gate3.8-rtl-verification`, ahead 0, behind 0.
- Tool: Vitis HLS/Vivado XSim 2021.2, `xczu7ev-ffvc1156-2-e`, 10 ns target.
- Canonical implementation: `src/frontend.cpp`, `src/decode.cpp`, `src/branch.cpp`, `src/reset.cpp`, `include/boom_types.hpp`, `include/boom_state.hpp`, and `include/boom_interfaces.hpp`.
- Focused wrapper: `src/synth_module_tops.cpp::synth_frontend_top`.
- Generated binding: `scripts/generate_merged.sh` produces `src/boom_core_merged.cpp`; synthesis consumes that generated file.
- Canonical focused RTL: `boom_hls_gate5_1_frontend_synth_frontend_top/solution_module/syn/verilog/`.
- Accepted Gate 4.1 evidence baseline: `reports/gate4_1/m3/m3c/`.
- `src/boom_all.cpp` is dirty before this task but remains legacy, non-canonical, excluded, and untouched.
- Other pre-existing dirty tracked files were three logs plus `vitis_hls.log`; untracked build/report artifacts were not baseline evidence.
- No implementation source was changed during focused RTL acceptance. Only the testbench, runner, reports, and documentation were added or updated.

The existing focused top exposes IMEM streams, `seed`, and PC only. It does not expose redirect, ROB ownership, backend stall, held instruction/fault, Decode output, or runtime reset controls. This interface limitation is an acceptance blocker, not a waived requirement.
