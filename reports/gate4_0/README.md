# Gate 4.0 Evidence

Gate 4.0 proceeds through W0-W6. Run `python3 scripts/gate4_0/extract_smallboom_topology.py` to regenerate the W0 topology freeze from the sibling SmallBoom generated artifacts.

W0 establishes that the integer-only implementation scope contains one MEM lane and one INT lane. The third configured queue is FP and is not counted as an integer issue lane.

- W1: `W1_FIXED_LANE_INTERFACE_VERIFIED`; fixed three-lane issue/result interfaces with one implemented acceptance lane. See `w1/w1_results.md`.
- W2: `W2_DUAL_SELECTION_VERIFIED`; simultaneous oldest-ready MEM/INT selection with at most one accepted grant. See `w2/w2_results.md`.
- W3: `W3_DUAL_EXECUTE_ACCEPTANCE_VERIFIED`; fixed MEM/INT dual acceptance and execution with retained, serialized completion/writeback. See `w3/w3_results.md`.
- W4C historical checkpoint: `W4C_MULTI_WAKEUP`; three fixed integer wakeup ports, three fixed bypass ports, and one serialized PRF write/commit. See `w4/w4c_stage_results.md`.
- W4E final: `W4_MULTI_WRITEBACK_VERIFIED`; three completion slots, three wakeup/bypass ports, two physical PRF writes, focused RTL 20/20, current W3 11/11, full-core 49/49, and source-bound csynth 7/7. See `w4/w4_results.md`.

W2 is a selection checkpoint, not dual execution and not an accepted product PPA configuration. Peak generated grants are two, peak accepted grants remain one, and the FP lane remains invalid.

W3 passes 400/400 software checks, persistent random 100x64 with no drop or duplicate, focused generated RTL 11/11, full-core XSim/trace comparison 49/49, five csynth targets, and synthesis guardrails. `READY_FOR_W4_MULTI_WAKEUP_WRITEBACK=true` authorizes only the next experiment; no W4 implementation is present. `READY_FOR_OFFICIAL_GATE_3=false`, M009 remains `PARTIALLY_VERIFIED`, and M014 remains `VERIFIED`. Official Chipyard/FESVR/DRAMSim full-system validation remains unavailable.

The canonical final W4 flows are `scripts/gate4_0/run_w4e_regressions.sh`, `scripts/gate4_0/run_w4_rtl.sh`, `scripts/gate4_0/run_w4_full_core_rtl.sh`, and `scripts/gate4_0/run_w4e_csynth.sh`. W4A-W4D reports remain historical stage evidence; commit width remains one.
