# L1 Behavior Equivalence

Historical checkpoint record: this report captures preservation status when
focused RTL acceptance was `17/80`. It is validation-architecture provenance,
not the current gate verdict. See `l1_results.md` for the final `80/80` status.

- Minimal LSU native: 14/14 PASS
- W3 completion native: 18/18 PASS
- W3 canonical regression: all 15 recorded suites PASS, including LSU/reset/branch and random
- Current-source default RTL LSU/reset/backpressure subset: 17/17 PASS
- PF1 exception directed/random/program: PASS
- PF3/PF4/PF5 native: PASS
- PF3 current-source full-core RTL: 12/12 PASS
- PF5 current-source full-core RTL: 12/12 PASS; mandatory faults 2/2
- PF6 current-source integrated RTL: 12/12 PASS; mandatory faults 2/2
- W4 current-source multi-writeback: 13/13 PASS
- Current-compatible RVC native, CSim, and full-core RTL: 11/11 PASS each

`L1_DEFAULT_8_8_BEHAVIOR_EQUIVALENT=true` for exercised behavior. Full requested preservation was complete at this checkpoint. Gate acceptance was then blocked only by the required focused RTL threshold: 17/80 accepted cases versus 80 required.
