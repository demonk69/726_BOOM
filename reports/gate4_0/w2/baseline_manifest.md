# Gate 4.0 W2 Baseline Freeze

- W1 reference: `fa3dbcc`
- Resolved immutable W1 commit: `fa3dbcc8d9559d0da5a0aa8d401816404124fb99`
- Current commit at freeze: `fa3dbcc8d9559d0da5a0aa8d401816404124fb99`
- Baseline source: committed blobs from the immutable W1 commit, never worktree copies

## Frozen Baseline

- Production C/C++ source identities: `source_hashes_before.txt`.
- W1 csynth: 5.898 ns, 51558 LUT, 12802 FF, 12 BRAM_18K, 3 DSP; `CORE_CYCLE` unpipelined.
- W1 traces: 14 total, comprising seven C++ and seven csim traces; the frozen five-program pair is 10/10 byte-identical.
- Gate 3.9 RTL traces: 49/49 PASS; matrix SHA-256 `f639bc15f89aa5f1a9ffcb146fdd7b632f95ac2f8cfb02062b10be7040324add`.
- W1 unit/regression assertions: 149 passed, 0 failed; architectural checks 10/10 PASS.

## Pre-existing Dirty Worktree Groups

- modified tracked logs: 2 porcelain status entries
- untracked backup logs: 110 porcelain status entries
- untracked build tree: 1 porcelain status entry
- untracked generated report binaries/artifacts: 1 porcelain status entry

Dirty paths are grouped by type only. Unrelated log names and contents are intentionally not copied into this freeze, and no pre-existing path is modified.

## Artifacts

- `baseline_manifest.md`: commit, baseline summary, and grouped pre-existing worktree state.
- `source_hashes_before.txt`: SHA-256 identities of committed production sources under `src/` and `include/`.
- `regression_before.md`: committed W1 test, trace, architectural, and csynth outcomes.
- `w1_resource_baseline.csv`: W1 csynth resources plus immutable report identities.
- `w1_trace_manifest.csv`: immutable identities for W1 C++/csim and Gate 3.9 RTL traces.

## Reproduction

Run `python3 scripts/gate4_0/freeze_w2_baseline.py` from anywhere in this repository. The script writes only these five W2 files and excludes them and itself from dirty-state grouping.

## Limitations

- This is an evidence freeze, not a rerun of tests, csim, synthesis, or RTL simulation.
- W1 `load_store` and `tohost` C++/csim traces are inventoried but were not members of the committed 10-trace byte-comparison set.
- Official Chipyard/FESVR/DRAMSim equivalence remained unavailable in W1; the recorded full-program result uses the committed HLS architectural comparison.
