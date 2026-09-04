# Gate 5.4 PR0 Baseline Manifest

## Checkpoints

- Gate 5.3 accepted B3I commit: `a48e527f78e42945969e945c63f501641eae179c`.
- Gate 5.3 final review commit: `ef051ca4e3673d663f9d10b55029c956fbc0052a`.
- Current committed baseline/PF0: `490788d8f20a43b802f5457d9e4fb55915a2a9e7`.
- PF1 is a dirty-worktree product change over PF0. Its relevant source hashes are recorded in `current_runner_manifest.md`.
- Reproduction roots: `/tmp/boom_hls/pr0/accepted` and `/tmp/boom_hls/pr0/current`.
- Compiler: `g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0`.

The accepted and current committed baseline hashes are identical for both random tests, both Gate 5.3 runner scripts, and `src/frontend.cpp`, `src/fetch_packet.cpp`, `src/fetch_buffer.cpp`, `src/rvc.cpp`, `src/decode.cpp`, and `src/divider.cpp`. There is no post-acceptance source or test drift in this scope.

## Frozen Evidence

- Gate 5.3 Final records B3I persistent random `256 x 4096`, all error counters zero.
- The accepted log records `drop=0`, `order_error=0`, and every other error counter zero.
- PF1 records the B2 integration random marker with `drop=425388` and `ordering_error=174170` for both PF1 and baseline-equivalent builds.
- `src/boom_all.cpp` was excluded and not read as a source input, compiled, or modified. Its before/after SHA-256 guard is `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.

## Hygiene

All binaries, worktrees, diagnostic instrumentation, and logs are under `/tmp/boom_hls/pr0/`. Repository changes from PR0 are limited to `reports/gate5_4_preservation/pr0/`.
