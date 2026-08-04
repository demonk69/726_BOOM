# Gate 4.0 W4 Baseline Freeze

- Scope: W4 baseline freeze and real SmallBoomConfig topology extraction only.
- Baseline commit: `d0de7f560e8d319eefcfa78af0ed26da18bcc971` (`Gate 4.0 W3: verify dual MEM/INT execution`).
- Baseline verification: `git rev-parse HEAD` returned the exact required commit before report creation.
- W4A implementation: not started; no implementation source was edited.
- Existing dirty state: preserved. The tracked paths already dirty were `reports/equivalence/provisional_gate3/hls_csim_trace.log`, `src/boom_all.cpp`, and `vitis_hls.log`; 279 collapsed untracked entries were also present (`git status --short --untracked-files=normal` reported 282 lines total).
- Explicit exclusion: `src/boom_all.cpp` was neither read nor used as implementation evidence. It is a pre-existing modified legacy snapshot and remains untouched.
- W1-W3 preservation: no file under `reports/gate4_0/w1`, `reports/gate4_0/w2`, or `reports/gate4_0/w3` was written or overwritten.

## Frozen Inputs

| Input | SHA-256 | Result |
|---|---|---|
| `reports/gate4_0/execution_topology.csv` | `05d8aac09c9f2d9af224d9ccda44219e4e3ff9bb039d16c84f5e628bfcc7ee98` | read and cross-checked |
| `reports/gate4_0/fu_port_mapping.csv` | `f8cf883de36ff948f8a77bde38be0ff0fcd323c9d05a7829272bfabf5c6877ff` | read and cross-checked |
| `reports/gate4_0/prf_port_inventory.csv` | `60a250ec4190ffde22d938a4324dd43707365893377082fe5a69ffc788919f86` | read and cross-checked |
| `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.top.v` | `6a3539a09e77f4c01a72269517f05cf8c7fdbd667e4c130116d741a5c2f3f3a9` | primary signal-level evidence |
| `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.fir` | `d55c6acb0faa2e77b737b56c7402d5181409f37d12a5d4c3e5ccce58799b038e` | located and hashed |
| `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.top.fir` | `f6ddd22310fa2b5227b4f35a725a5226ff694830edb220b19cb30d26a47fd524` | located and hashed |

FIRRTL signal cross-check: `chipyard.TestHarness.SmallBoomConfig.fir:275143-275154` contains integer wakeup ports 0-2, lines 277735-277757 contain integer PRF writes 0-1, and lines 277808-277855 contain ROB writeback responses 0-3. These agree with the concrete Verilog module ports and assignments recorded in the topology CSVs.

## W3 Validation

All 21 nonrecursive entries in `reports/gate4_0/w3/artifact_manifest.csv` matched their recorded hashes. All 42 entries in `reports/gate4_0/w3/source_hashes_after.txt` returned `OK`. The trace-only hashes for `trace_comparison.csv` and `full_program_architectural_diff.csv` also matched. No regression or synthesis was rerun because this work item is a baseline freeze, not W4A.
