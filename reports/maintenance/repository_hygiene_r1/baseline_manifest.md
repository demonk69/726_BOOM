# Repository Hygiene R1 Baseline

- Requested workspace: `/home/lab_726/boom`
- Actual Git root: `/home/lab_726/boom/hls_boom`
- Branch: `gate3.8-rtl-verification`
- Baseline HEAD: `5bc5644b1b47697a439ce1605b87068b76eee68d`
- HEAD subject: `Gate 5.4 P2: verify standalone predictor foundation`
- P2 committed: true
- Initial workspace bytes: `14633529344`
- Initial workspace GiB: `13.629`
- Initial tracked dirty paths: 38
- Active Vitis HLS/Vivado/XSim/compiler workspaces: 0

The requested workspace parent is not itself a Git worktree. All Git protection checks therefore use the actual `hls_boom/` root, while disk inventory covers the complete requested parent. Paths outside the Git root are classified `UNKNOWN` and were not deletion candidates.

`git_status_before.txt`, `git_dirty_inventory.csv`, and `git_tracked_inventory.csv` freeze the protection baseline. `src_boom_all_before.sha256` and `functional_source_before.sha256` freeze protected source content before deletion. The `.gitignore` was already modified at baseline, so no ignore rule was changed by R1.

Accepted Gate reports and all content under `reports/` were protected. Report path references were scanned before candidate decisions. A textual reference forced `KEEP`, even where the target was an otherwise reproducible HLS workspace.
