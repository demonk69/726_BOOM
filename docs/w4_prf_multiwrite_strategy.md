# W4D Integer PRF Multiwrite Strategy

## Contract

The canonical product keeps `INT_PHYS_REGS=52`, four logical PRF reads, three wakeup/bypass buses, and commit width one. Physical integer writeback is exactly `WritebackEvent writebacks[2]`. Completion selects by wrap-safe ROB age and then fixed source order LSU load, MEM execute, INT execute.

Two different nonzero destinations may update the logical integer PRF in one core cycle. Equal destination/equal value events coalesce to one physical write while both selected ROB owners complete. Equal destination/different value events produce no write or wakeup and become deterministic precise validation faults with cause `WRITEBACK_VALIDATION_FAULT_CAUSE`. Participants leave busy/pending state, the oldest owner identifies the sticky fault status, commit emits one exception trace and asserts `io_trap`, and reset clears the fault. x0 is never presented as a write.

## HLS Mapping

The final source implements one logical 52-entry PRF with two replicated 52x64 data banks and a packed 52-bit Live Value Table. Vitis infers two distinct `RAM_AUTO_1R1W` instances, `state_int_rf_bank0_U` and `state_int_rf_bank1_U`. Physical port 0 writes only `int_rf_bank0`; physical port 1 writes only `int_rf_bank1`. A port write updates the destination's LVT bit to identify its bank. `prf_read` reads the selected replica, and `prf_seed` initializes both replicas coherently. Because destinations are not parity-banked, any two distinct destinations, including two even or two odd physical register numbers, may write concurrently.

Storage replication is intentional and explicit. The banks are coherent through the LVT, not by mirroring every runtime write. There remains exactly one logical 52-entry architectural PRF and no width/capacity reduction. No complete partition, DATAFLOW region, false-dependence pragma, or synthesis-only semantic path is used. Native C++, Vitis csim, and generated RTL execute the same bank/LVT operations.

W4C generated one `state_int_rf_RAM_AUTO_1R1W`. The prior single-array W4D mapping required an impermissible dependence waiver to expose two writes. The final LVT implementation removes that waiver and reports target II=1/final II=1. Generated top RTL has separate `state_int_rf_bank0_we0` and `state_int_rf_bank1_we0` signals driving separate bank instances.

This exact structure is present in `synth_completion_top`, `synth_core_step_top`, and `boom_core_top`. `scripts/gate4_0/run_w4e_csynth.sh` fails unless all generated product views contain both physical write enables. Exact paths are in `reports/gate4_0/w4/prf_after.csv`.

The source-bound waiver-free W4E rerun verifies the two bank enables in `synth_completion_top`, `synth_core_step_top`, and `boom_core_top`. The final status is `W4_MULTI_WRITEBACK_VERIFIED=true`.

## Read-After-Write

Wakeup and bypass values are built from validated completion payloads before physical writeback. Issue operand lookup is x0, then current validated bypass, then nonbusy `prf_read`. Thus a same-cycle consumer gets completion data directly, not tool-dependent bank read-during-write behavior. Completion stops after servicing any branch or exception, so a younger result suppressed from that cycle's wakeup snapshot remains pending and publishes its wakeup before writing on the next cycle. Only a selected or coalesced write clears ordinary writer busy state. Commit uses `prf_read`; generated RTL inspection and C++/csim trace equality audit the ordering. The branch pre-resolution and branch-marker clearing in `synth_w3_branch_kill_top` is a diagnostic-wrapper guard that prevents the legacy W3 service from resolving the same synthetic branch twice; it is not present in `boom_core_step` or any canonical product path.

Runtime reset preserves existing PRF contents, clears stale in-flight ownership elsewhere, and forces x0 to zero in both replicas. Branch recovery invalidates younger owners and pending publications before writes; it does not need to roll back already valid PRF bank/LVT state.
