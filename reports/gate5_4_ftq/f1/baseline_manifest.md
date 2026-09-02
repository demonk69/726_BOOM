# Gate 5.4 F1 Baseline

- Branch: `gate3.8-rtl-verification`
- HEAD: `5bc5644b1b47697a439ce1605b87068b76eee68d`
- P2 commit present: yes
- Dirty baseline: captured in `git_status_before.txt`
- Repository hygiene R1: pass
- Build root: `${BOOM_BUILD_ROOT:-/tmp/boom_hls}`
- Build retention: only when `BOOM_KEEP_BUILD=1` or a runner fails
- Protected product source hashes: `source_hashes_before.txt`
- Legacy `src/boom_all.cpp`: pre-existing dirty file, hash `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`, excluded

F1 is standalone. No Frontend, Fetch Packet, Fetch Buffer, Decode, Rename,
Dispatch, ROB, Commit, Execute, branch recovery, or product predictor path is
connected or modified.
