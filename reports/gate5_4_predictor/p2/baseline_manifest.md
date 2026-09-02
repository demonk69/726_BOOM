# Gate 5.4 P2 Baseline Manifest

- HEAD: `ef051ca4e3673d663f9d10b55029c956fbc0052a`.
- Branch: `gate3.8-rtl-verification`.
- Worktree: dirty before P2; the complete captured status, including historical tracked and untracked state, is `git_status_before.txt`.
- Accepted prerequisites: F0 FTQ review, P0 predictor-interface review, and P1 CFI predecode verification.
- Standalone core API scope: `include/predictor.hpp` and `src/predictor.cpp` only.
- Protected product hashes: `source_hashes_before.txt` and `source_hashes_after.txt` are identical for Frontend, Fetch Packet, Fetch Buffer, Decode, Branch, ROB, Execute, and `src/boom_all.cpp`.
- `src/boom_all.cpp` remains a legacy excluded source and was not modified by P2.
- Product implementation, tests, scripts, and product sources are not modified by this documentation closeout.

The P2 report consumes existing source, native logs, XSim logs, generated RTL, and Vitis HLS XML. It does not represent a clean-worktree baseline and does not discard or normalize pre-existing changes.
