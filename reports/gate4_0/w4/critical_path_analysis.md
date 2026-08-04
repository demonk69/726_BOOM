# Gate 4.0 W4 Final Critical Path

Vitis HLS 2021.2 estimates both `synth_core_step_top` and `boom_core_top` at **6.025 ns** against 10.00 ns, a 3.975 ns estimated margin. This is HLS scheduling, not post-route STA.

The verbose state-local report identifies `execute_module`, state 8, at **6.02 ns**. The cone now includes the LVT bank select after parallel reads of the replicated 52x64 banks, followed by lane/opcode/operand selects and execute result/control generation. Completion arbitration remains secondary.

Canonical verbose evidence is `csynth_final/{synth_core_step_top,boom_core_top}/verbose/execute_module.verbose.sched.rpt`; state 8 and operations 218-235 expose the PRF-read and operand-select cone. Top-level reports are retained beside it.

All seven tops report `PipelineType=no`. `boom_core_top` contains `CORE_CYCLE` with no `PipelineII`. No DATAFLOW, false-dependence, or explicit complete-partition directive is present. The local `apply_writeback_ports` target/final II is 1 and generated RTL exposes independent `bank0_we0` and `bank1_we0`.
