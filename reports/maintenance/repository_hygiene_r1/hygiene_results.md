# Repository Hygiene R1 Results

## Result

```text
REPOSITORY_HYGIENE_R1_PASS=true
P2_COMMITTED=true
DISK_USAGE_BEFORE_GB=13.629
DISK_USAGE_AFTER_GB=13.107
DISK_SPACE_RECLAIMED_GB=0.522
DELETE_PATH_COUNT=49
DELETED_TRACKED_FILES=0
DELETED_DIRTY_FILES=0
DELETED_ACCEPTED_EVIDENCE=0
DELETED_ACTIVE_WORKSPACES=0
PREEXISTING_DIRTY_LEFT_UNTOUCHED=true
SRC_BOOM_ALL_UNTOUCHED=true
CANONICAL_FUNCTIONAL_SOURCE_CHANGED=false
BOOM_BUILD_ROOT_DEFAULT=/tmp/boom_hls
CLEANUP_SCRIPT_DEFAULT_DRY_RUN=true
WORKSPACE_WARNING_THRESHOLD_GB=20
WORKSPACE_SEVERE_THRESHOLD_GB=50
GITIGNORE_UPDATE_DEFERRED=true
READY_FOR_GATE5_4_F1_STANDALONE_FTQ=true
```

No F1, FTQ implementation, predictor integration, or ICache work was started.

## Cleanup

The dry run classified 49 exact paths as `DELETE`, totaling `560680960` bytes. Its protection counters were all zero:

```text
SAFE_DELETE_TRACKED_FILES=0
SAFE_DELETE_DIRTY_FILES=0
SAFE_DELETE_ACCEPTED_EVIDENCE=0
SAFE_DELETE_ACTIVE_WORKSPACES=0
SAFE_DELETE_SYMLINK_PATHS=0
```

Only those manifest paths were removed. The final measured workspace delta was `560291840` bytes; newly written R1 reports account for the difference from the candidate total. No `git clean`, reset, restore, checkout, stash, wildcard build deletion, or report deletion was used.

The largest retained temporary tree is `build/gate5_3_fetch_buffer` at approximately 6.0 GiB. It and the P1/P2 HLS projects are `KEEP` because accepted report text references their paths. `reports/` remains approximately 2.3 GiB and was not cleaned because accepted and unconfirmed evidence is protected. `duplicate_artifacts.csv` is advisory only; duplicates under reports are explicitly `KEEP` and other duplicates are `REVIEW`.

## Protection Verification

- `deleted_tracked_files.txt` is empty.
- All 38 pre-existing tracked dirty paths have identical before/after status in `dirty_status_comparison.csv`.
- `src/boom_all.cpp` before/after SHA-256 records match.
- Frozen canonical functional source before/after SHA-256 records match.
- No active Vitis HLS, Vivado, XSim, XElab, XVLog, GCC, or G++ workspace was identified at deletion time.
- Historical reports and paths were not moved or renamed.

`PREEXISTING_DIRTY_LEFT_UNTOUCHED` refers to protected source/evidence and Git status preservation. Existing untracked maintenance-policy files were intentionally completed because they are explicit R1 deliverables; they remain untracked rather than being staged.

## Future Policy

`scripts/common/build_env.sh` defines `BOOM_BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls}`, unique stage directories, success cleanup, and `BOOM_KEEP_BUILD=1` preservation. `scripts/common/gate_workspace.sh` exposes `gate_begin`, `gate_build_dir`, `gate_preserve_failure`, and `gate_cleanup_success` for future runners. Historical runners were not migrated.

`scripts/maintenance/cleanup_workspace.sh` defaults to dry-run and accepts `--execute`, `--older-than-days N`, and `--build-root PATH`. It is build-root bounded and rejects tracked, dirty, symlink-containing, or active paths. `scripts/maintenance/check_workspace_size.sh` reports repository/build-root size and largest directories/files, warning at 20 GiB and severe at 50 GiB without automatic deletion.

The canonical layout and artifact retention rules are in `docs/repository_layout.md`, `docs/gate_artifact_policy.md`, and the audited `docs/workspace_storage_policy.md`. `.gitignore` already had relevant dirty additions at baseline, so R1 did not mix further changes into it.

## Verification

- Shell syntax: PASS
- Common Gate workspace lifecycle smoke test: PASS
- Cleanup default dry-run: PASS
- Protected source hashes: PASS
- Tracked dirty status comparison: PASS
- R1-file whitespace check: PASS
- Whole-worktree `git diff --check`: blocked by pre-existing trailing whitespace in historical logs/CSV; those accepted/dirty files were left unchanged
