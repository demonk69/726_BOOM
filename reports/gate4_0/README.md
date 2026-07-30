# Gate 4.0 Evidence

Gate 4.0 proceeds through W0-W6. Run `python3 scripts/gate4_0/extract_smallboom_topology.py` to regenerate the W0 topology freeze from the sibling SmallBoom generated artifacts.

W0 establishes that the integer-only implementation scope contains one MEM lane and one INT lane. The third configured queue is FP and is not counted as an integer issue lane.

- W1: `W1_FIXED_LANE_INTERFACE_VERIFIED`; fixed three-lane issue/result interfaces with one implemented acceptance lane. See `w1/w1_results.md`.
- W2: `W2_DUAL_SELECTION_VERIFIED`; simultaneous oldest-ready MEM/INT selection with at most one accepted grant. See `w2/w2_results.md`.

W2 is a selection checkpoint, not dual execution and not an accepted product PPA configuration. Peak generated grants are two, peak accepted grants remain one, and the FP lane remains invalid.
