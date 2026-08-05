# Gate 4.1 M2C Handoff

## Frozen state

- M1: M1_RV64M_DECODE_VERIFIED
- M2A: M2A_STANDALONE_MUL_ARITHMETIC_VERIFIED
- M2B: M2B_INT_INTEGRATION_FUNCTIONALLY_VERIFIED
- M2B_PPA_BLOCKER=true
- M2_MUL_FAMILY_VERIFIED=false
- READY_FOR_M3_DIVIDER_CORE=false

## Functional evidence

- Multiply integration directed: 30/30 PASS
- Arithmetic/random integration: 131072 vectors, 0 mismatch
- Held-result checks: 16384, 0 failure
- W3 legacy: 400/400 PASS
- W4 directed: 95/95 PASS
- Focused RTL: 10/10 PASS
- Full-core RTL M programs: 2/2 PASS
- Completion/ROB/LSU topology unchanged
- Divider not started
- CORE_CYCLE unpipelined

## Synthesis

- synth_mul_top: 6.463 ns
- synth_execute_top: 7.201 ns
- synth_completion_top: 5.474 ns
- synth_core_step_top: 7.257 ns, 33 DSP
- raw boom_core_step rejected because aggregate interfaces exceed 4096 bits

## M2C goal

Reduce multiply DSP and timing cost without changing:
- arithmetic results
- latency
- INT-lane behavior
- W4 completion topology
- PRF ports
- wakeup/bypass ports
- ROB completion ports

Do not:
- start Divider
- pipeline CORE_CYCLE
- use DATAFLOW or false dependence
- modify src/boom_all.cpp
- retry raw boom_core_step as the product synthesis top
