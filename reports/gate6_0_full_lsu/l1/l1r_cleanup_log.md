# G6.0 L1R Cleanup Log

Cleanup classification: `EPHEMERAL_BUILD_PROVENANCE` only. Product source, test source, scripts, and all durable reports are protected.

## Pre-Cleanup

```text
ROOT_FS_BEFORE_AVAIL=0
ROOT_FS_BEFORE_USE_PERCENT=100%
L1R_ACTIVE_PROCESS_COUNT_AFTER_PAUSE=0
```

Largest ephemeral path before cleanup:

```text
/tmp/boom_hls/g6_l1_closure/focused approximately 12G
/tmp/boom_hls/g6_l1_closure/integrated approximately 674M
/tmp/boom_hls/g6_l1_closure/canonical approximately 618M
/tmp/boom_hls/g6_l1_closure/pf3 approximately 56M
/tmp/boom_hls/g6_l1_closure/rvc_rtl approximately 56M
/tmp/boom_hls/g6_l1_closure/rvc_csim approximately 8.3M
/tmp/boom_hls/g6_l1_closure approximately 14G total
```

## Removed Paths

| Path | Approximate size | Reason | Classification |
|---|---:|---|---|
| `/tmp/boom_hls/g6_l1_closure` | 14G | Paused L1R Vitis projects, generated RTL, XSim state, executables, and incomplete focused synthesis | `EPHEMERAL_BUILD_PROVENANCE` |
| `build/` | 2.0G | Ignored repo-local historical build outputs; Git reported zero tracked files | `EPHEMERAL_BUILD_PROVENANCE` |
| `boom_hls_gate5_2_rvc_r2_repair_*` | approximately 1.8G | Old Gate 5.2 HLS projects; Git reported zero tracked files and no status entries | `EPHEMERAL_BUILD_PROVENANCE` |

Durable L1R summaries, matrices, traces, source manifests, and logs under `reports/gate6_0_full_lsu/l1/` were retained.

## Post-Cleanup Verification

```text
ROOT_FS_AFTER_AVAILABLE_BYTES=14282964992
ROOT_FS_AFTER_AVAIL_HUMAN=14G
ROOT_FS_AFTER_USE_PERCENT=70%
ROOT_FS_AFTER_INODE_USE_PERCENT=15%
TMP_BOOM_HLS_AFTER=4.0K
L1R_ACTIVE_PROCESS_COUNT=0
INDEX_ENTRIES=0
TRACKED_DELETIONS=0
SOURCE_MANIFEST_FILES_CHECKED=41
SOURCE_MANIFEST_MISMATCHES=0
MERGED_SOURCE_SHA256=76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c
SRC_BOOM_ALL_SHA256=d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c
L1_IMPLEMENTATION_PRESERVED=true
L1_MEMORY_BEHAVIOR_CHANGED=false
CLEANUP_STATE=COMPLETE_WITH_SPACE_TARGET_LIMITATION
```

The exact post-cleanup free space is about 14.28 decimal GB, below the requested 15 GB minimum. No further clearly classified repo-local ephemeral build directory large enough to close the gap remained. General user caches outside the repository were deliberately not removed because they are unrelated user data.

## Global Cleanup Phase

```text
GLOBAL_EPHEMERAL_CLEANUP=FAIL_SPACE_TARGET_NOT_REACHED
L1R_TASK_STATE=PAUSED
SCANNED_ROOTS=/home/lab_726,/tmp,/var/tmp
ACTIVE_EDA_PROCESS_COUNT=0
PROTECTED_ACTIVE_WORKSPACES=NONE
GIT_ROOTS_DISCOVERED_UNDER_HOME=39
ROOT_FS_BEFORE_AVAILABLE_BYTES=14282940416
ROOT_FS_BEFORE_USED_BYTES=32090943488
ROOT_FS_BEFORE_USE_PERCENT=70%
ROOT_FS_AFTER_AVAILABLE_BYTES=14290341888
ROOT_FS_AFTER_USED_BYTES=32083542016
ROOT_FS_AFTER_USE_PERCENT=70%
ROOT_FS_AFTER_INODE_USE_PERCENT=15%
ROOT_FS_FREED_BYTES=7401472
ROOT_FS_FREED_GB=0.0074
HOME_VOLUME_AFTER_AVAILABLE_BYTES=430422777856
HOME_VOLUME_AFTER_AVAIL_HUMAN=401G
REMOVED_ALLOCATED_BYTES_TOTAL=11367325653
REMOVED_ALLOCATED_GIB_TOTAL=10.59
REMOVED_EPHEMERAL_PATH_COUNT=8
REMOVED_HLS_WORKSPACE_COUNT=1
REMOVED_XSIM_WORKSPACE_COUNT=1
REMOVED_CSIM_WORKSPACE_COUNT=1
REMOVED_GENERATED_BUILD_COUNT=6
DELETE_CANDIDATE_TRACKED_FILES=0
DELETE_CANDIDATE_ACTIVE_WORKSPACES=0
DELETE_CANDIDATE_DURABLE_REPORTS=0
DELETE_CANDIDATE_UNKNOWN=0
TRACKED_FILES_DELETED=0
DURABLE_REPORTS_REMOVED=0
OTHER_REPOSITORIES_TRACKED_FILES_REMOVED=0
L1_SOURCE_HASH_PRESERVED=true
L1_IMPLEMENTATION_PRESERVED=true
EXISTING_PASS_EVIDENCE_PRESERVED=true
PRODUCT_SOURCE_CHANGED_BY_CLEANUP=false
SRC_BOOM_ALL_UNCHANGED=true
PREEXISTING_DIRTY_LEFT_UNTOUCHED=true
INDEX_EMPTY=true
SOURCE_REPO_PRESERVED=true
GENERATED_WORKSPACE_REMOVED=true
OPTIONAL_CACHE_CLEANUP_GB=31
OPTIONAL_CACHE_FILESYSTEM=/home
READY_TO_RESUME_G6_0_L1R=false
NEXT_REQUIRED_ACTION=RESUME_G6_0_L1_VALIDATION_CLOSURE
```

The cleanup inventory and dry-run delete manifest are `/tmp/boom_global_cleanup_inventory.tsv` and `/tmp/boom_global_cleanup_delete_manifest.tsv`.

### Mount Topology

`/` is `/dev/nvme0n1p1` while all `/home/lab_726` candidates and the current repository are on `/dev/nvme0n1p3`. Removing 10.59 GiB of old generated RTL/compiler output from `/home` increased that volume's free space to 401G but could not increase root free space. Only the selected `/tmp` workspaces affected `/`, releasing about 7.4 MB. The 15 GB root minimum cannot be reached from the permitted project-ephemeral paths found under `/tmp` and `/var/tmp`; unrelated system data and general caches were not touched.

### Largest Consumers Scanned

| Path | Size | Disposition |
|---|---:|---|
| `/home/lab_726/Xilinx` | 90.74G | Protected tool installation |
| `/home/lab_726/opencode_tmp` | 57.19G before cleanup | Mixed tree; only exact generated leaves considered |
| `/home/lab_726/.local` | 43.62G | General user data; retained |
| `/home/lab_726/.cache` | 30.28G | Optional general cache on `/home`; retained |
| `/home/lab_726/verification_work` | 6.02G candidate content before cleanup | Mixed tree; only exact generated leaf considered |
| `/home/lab_726/boom/hls_boom/reports` | 2.4G | Protected durable evidence |
| `/tmp/boom_hls_review` | 5.71M before cleanup | Inactive L1 CSim child removed |
| `/var/tmp/OptixCache_lab_726` | Not selected | UNKNOWN/non-BOOM cache; retained |

### Removed Directories

| Path | Size in bytes | Classification |
|---|---:|---|
| `/home/lab_726/opencode_tmp/gemm_large_k1024_all_shapes_clk_d412efc/work` | 6755382750 | `COMPILER_BUILD_TREE` |
| `/home/lab_726/verification_work/coverage_line_suite/work` | 3125252272 | `COMPILER_BUILD_TREE` |
| `/home/lab_726/opencode_tmp/vgg_parallel_20260707_162431/core4/repo/sim/vgg_mc_closed_loop/obj_dir` | 1479276871 | `COMPILER_BUILD_TREE` |
| `/tmp/boom_hls_review/g6_l1/csim/4_4/g6_l1_csim_4_4` | 5971968 | `CSIM_EPHEMERAL_WORKSPACE` |
| `/tmp/opencode/g6_l1r_final` | 434176 | `XSIM_EPHEMERAL_WORKSPACE` |
| `/tmp/opencode/g6_l1r_focused_2` | 335872 | `COMPILER_BUILD_TREE` |
| `/tmp/opencode/g6_l1r_redesign` | 335872 | `COMPILER_BUILD_TREE` |
| `/tmp/opencode/g6_l1r_redesign_final` | 335872 | `COMPILER_BUILD_TREE` |

All three home-volume candidates had strong Verilator markers (`obj_dir`, `Vtb_*`, generated C++, object files, archives, and makefiles), owner `lab_726`, no enclosing Git root, and no active open handles. Every `/tmp` candidate likewise had explicit Vitis, XSim, or L1R generated-build markers and no active process.

### Integrity Verification

The current BOOM repository remains intentionally dirty with 1207 status entries observed after cleanup; cleanup did not normalize or revert the worktree. Its index is empty and it has zero tracked deletions. All 41 files represented by the accepted current-source manifest match their recorded hashes.

```text
L1R_CURRENT_SOURCE_HASH=b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618
SOURCE_MANIFEST_FILES_CHECKED=41
SOURCE_MANIFEST_MISMATCHES=0
MERGED_SOURCE_SHA256=76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c
SRC_BOOM_ALL_SHA256=d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c
```

## Root-Only Cleanup Phase

```text
ROOT_ONLY_CLEANUP=FAIL
L1R_TASK_STATE=PAUSED
ROOT_MOUNT=/dev/nvme0n1p1:/
HOME_MOUNT=/dev/nvme0n1p3:/home
TMP_MOUNT=/dev/nvme0n1p1:/
VARTMP_MOUNT=/dev/nvme0n1p1:/
ROOT_FS_BEFORE_AVAIL_BYTES=14290235392
ROOT_FS_BEFORE_AVAIL_GB=14.290235392
ROOT_FS_BEFORE_USE_PERCENT=70%
ROOT_FS_AFTER_AVAIL_BYTES=14290313216
ROOT_FS_AFTER_AVAIL_GB=14.290313216
ROOT_FS_AFTER_USE_PERCENT=70%
ROOT_FREED_SPACE_BYTES=77824
ROOT_FREED_SPACE_GB=0.000077824
ROOT_DELETED_OPEN_FILE_COUNT_BEFORE=0
ROOT_DELETED_OPEN_BYTES_BEFORE=0
ROOT_DELETED_OPEN_GB_BEFORE=0
ROOT_DELETED_OPEN_FILE_COUNT_AFTER=0
ROOT_DELETED_OPEN_BYTES_AFTER=0
ROOT_DELETED_OPEN_GB_AFTER=0
TERMINATED_STALE_EDA_PROCESSES=0
FREED_DELETED_OPEN_GB=0
ROOT_LAB726_FILES_OVER_100M=0
ROOT_EDA_WORKSPACE_COUNT=0
REMOVED_ROOT_EPHEMERAL_PATH_COUNT=0
REMOVED_ROOT_EPHEMERAL_GB=0
DELETE_ROOT_TRACKED_FILES=0
DELETE_ROOT_DURABLE_REPORTS=0
DELETE_ROOT_ACTIVE_WORKSPACES=0
DELETE_ROOT_UNKNOWN_PATHS=0
TOPLEVEL_SCAN_PERMISSION_DENIED=64
LARGE_FILE_SCAN_PERMISSION_DENIED=47
EDA_MARKER_SCAN_PERMISSION_DENIED=47
ROOT_DOCKER_USAGE_GB=0
DOCKER_CLEANUP_RECOMMENDED=false
SYSTEM_LOG_USAGE_BYTES=1822075420
SYSTEM_LOG_USAGE_GB=1.822
SYSTEM_LOG_CLEANUP_RECOMMENDED=true
L1_SOURCE_HASH_PRESERVED=true
L1_IMPLEMENTATION_PRESERVED=true
EXISTING_PASS_EVIDENCE_PRESERVED=true
INDEX_EMPTY=true
TRACKED_PRODUCT_FILES_REMOVED=0
DURABLE_REPORTS_REMOVED=0
PRODUCT_SOURCE_CHANGED_BY_CLEANUP=false
SRC_BOOM_ALL_UNCHANGED=true
PREEXISTING_DIRTY_LEFT_UNTOUCHED=true
READY_TO_RESUME_G6_0_L1R=false
NEXT_REQUIRED_ACTION=ROOT_SPACE_REQUIRES_USER_DECISION
```

### Root Inventory

`du -x` reported these top-level root-device consumers:

| Path | Size | Classification | Cleanup decision |
|---|---:|---|---|
| `/usr` | 16G | Installed system software | `SYSTEM_MANAGED`; do not clean automatically |
| `/var` | 8.8G | Mixed system state | `SYSTEM_MANAGED`; do not clean automatically |
| `/opt` | 2.6G | Installed applications/toolchains | `SYSTEM_MANAGED`; do not clean automatically |
| `/boot` | 467M | Boot files | `SYSTEM_MANAGED`; do not clean automatically |
| `/tmp` | 40M | Mixed temporary data | No remaining qualifying large BOOM/EDA workspace |
| `/var/tmp` | 4.8M | Mixed temporary data | No qualifying BOOM/EDA workspace |

The largest `/var` subtrees were `/var/lib/snapd` at 6.1G and `/var/log` at 1.7G. The largest individual log found was a 128 MiB systemd user journal; the journal tree is system-managed and was not modified. `/var/lib/docker` and `/var/lib/containerd` had no reported usage.

```text
ROOT_BLOCKER_1=/usr:16G:SYSTEM_MANAGED:SAFE_TO_CLEAN=false
ROOT_BLOCKER_2=/var/lib/snapd:6.1G:SYSTEM_MANAGED:REQUIRES_USER_APPROVAL
ROOT_BLOCKER_3=/opt:2.6G:SYSTEM_MANAGED:SAFE_TO_CLEAN=false
```

No root-device file owned by `lab_726` exceeded 100 MiB, and no `.Xil`, `xsim.dir`, or `.autopilot_db` directory was found on the readable part of the root filesystem. `lsof +L1` showed only deleted files on tmpfs or the separate `/home` device, not on root device `259,1`; no process was terminated.

Root-only inventories are retained at:

```text
/tmp/root_deleted_open_files.tsv
/tmp/root_lab726_large_files.tsv
/tmp/root_eda_workspace_inventory.tsv
/tmp/root_project_cleanup_delete_manifest.tsv
```

The delete manifest contains no candidates. Reaching 15 GB now requires a separately approved system-managed action such as journal cleanup, stale Snap revision cleanup, package-cache cleanup, or root filesystem resizing. None was performed.
