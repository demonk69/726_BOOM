# Gate 5.4 P0 Baseline Manifest

- Audit date: 2026-08-20.
- Requested repository: `/home/lab_726/boom`; actual nested Git worktree: `/home/lab_726/boom/hls_boom`.
- Verified branch: `gate3.8-rtl-verification`.
- Accepted Gate 5.3 commit: `ef051ca4e3673d663f9d10b55029c956fbc0052a`.
- Current Git verification: PASS in the nested worktree. HEAD is exactly the accepted Gate 5.3 commit.
- Dirty state: nonempty and preserved. The F0 report remains uncommitted; P0 did not auto-commit it. See `git_status_before.txt` and the complete historical list in `reports/gate5_4_ftq/f0/git_status_before.txt`.
- Scope: read-only architecture audit plus reports/docs. No Predictor, FTQ, ICache, BTB, BIM, RAS, history predictor, Full LSU, or FPU implementation.
- Canonical source: inspected modular headers/sources only. `src/boom_all.cpp` was excluded and not read, modified, or included.

Frozen accepted baseline:

```text
GATE5_3_FETCH_BUFFER_VERIFIED=true
GATE5_4_F0_FTQ_PREREQUISITES_REVIEWED=true
GATE5_3_CANONICAL_DEPTH=8
GATE5_3_CANONICAL_STORAGE=AUTO
GATE5_3_CANONICAL_RESET_POLICY=CONTROL_ONLY
GATE5_3_CANONICAL_PACKET_WIDTH=2
GATE5_3_PPA_BLOCKER=false
DECODE_WIDTH=1
DISPATCH_WIDTH=1
COMMIT_WIDTH=1
CORE_CYCLE_PIPELINED=false
```

Accepted Gate 5.3 PPA remains evidence only: 135953 LUT, 33373 FF, 16 BRAM, 3 DSP, 6.341 ns. P0 ran no synthesis and makes no new PPA claim.
