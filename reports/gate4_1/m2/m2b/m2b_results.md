# Gate 4.1 M2B Results

## Status

- `M2B_INT_INTEGRATION_FUNCTIONALLY_VERIFIED=true`
- `M2B_PPA_BLOCKER=true`
- `M2_MUL_FAMILY_VERIFIED=false`
- `M009=PARTIALLY_VERIFIED`
- `M014=VERIFIED`
- `READY_FOR_OFFICIAL_GATE_3=false`

M2B correctly integrates all five RV64M multiply operations into the existing INT lane and W4 completion path. Functional C++, csim, focused RTL, and full-core RTL gates pass. PPA acceptance does not pass, and the raw `boom_core_step` synthesis target has a Vitis HLS interface blocker.

## Functional Evidence

- Directed integrated execute and completion payload: 30/30 PASS.
- Persistent random integrated execute: 128 seeds x 1024 vectors = 131072; all uopcs covered; 16384 held-result checks; zero mismatch.
- Standalone M2A arithmetic regression: 51 directed and 131072 random vectors PASS.
- Native full-core multiply program: 10/10 architectural register checks PASS.
- Canonical W3 preservation: 400/400 PASS across 197 runs.
- Canonical W4 directed preservation: 95/95 PASS.
- W4 persistent completion random: 128 seeds and 16384 cycles PASS with fixed peaks of three sources, two PRF writes, three wakeups, and three bypasses.
- C++/csim normalized traces: 7/7 PASS; normalized architecture/event/cycle checks: 21/21 PASS; full-program architectural diff: 10/10 PASS.
- Focused generated `synth_execute_top` RTL: 10/10 PASS.
- Generated `boom_core_top` RTL multiply program: 2/2 scenarios PASS; normal and trace-backpressure traces each match 10 expected register values and reach `tohost=1`.

## Synthesis Evidence

All reports use Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, a 10 ns target, and `PipelineType=no`.

| Top | Result | Period ns | LUT | FF | BRAM | DSP |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `synth_mul_top` | PASS | 6.463 | 612 | 7 | 0 | 33 |
| `synth_execute_top` | PASS | 7.201 | 2538 | 580 | 8 | 33 |
| `synth_completion_top` | PASS | 5.474 | 37959 | 10779 | 8 | 0 |
| `synth_core_step_top` | PPA BLOCKER | 7.257 | 107681 | 25381 | 16 | 33 |
| `boom_core_step` | TOOL BLOCKER | unavailable | unavailable | unavailable | unavailable | unavailable |

The raw `boom_core_step(BoomCoreState&, PipeSignals&)` top is rejected before scheduling because Vitis HLS attempts to aggregate `state_completion`, `state_rob`, and `state_issue` beyond its 4096-bit interface limit. The synthesizable full-state diagnostic wrapper completes, but 7.257 ns exceeds the 6.5 ns M2B threshold. Per scope, no optimization was attempted.

## Explicit Answers

1. All five multiply uopcs execute through `boom::execute_mul`: yes.
2. MUL low-half semantics are correct: yes.
3. MULH signed high-half semantics are correct: yes.
4. MULHSU signed/unsigned high-half semantics are correct: yes.
5. MULHU unsigned high-half semantics are correct: yes.
6. MULW truncation and sign extension are correct: yes.
7. Original `MicroOp`, ROB index, and allocation identity are retained: yes.
8. Held INT results remain stable under backpressure: yes, 16384 random probes plus directed checks.
9. Existing INT completion, PRF, wakeup, bypass, and ROB paths are reused: yes.
10. Completion or writeback ports were added: no.
11. Divider or DIV/REM execution was implemented: no.
12. Merged source contains `mul.cpp` exactly once: yes.
13. Canonical software/csim preservation passes: yes.
14. Focused and full-core generated RTL passes: yes.
15. All five requested synthesis targets pass: no; four synthesize and raw `boom_core_step` hits the documented interface limit.
16. M2B meets the 6.5 ns full-state threshold: no; `synth_core_step_top` is 7.257 ns, therefore `M2B_PPA_BLOCKER=true`.
