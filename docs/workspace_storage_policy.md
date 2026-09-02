# Workspace Storage Policy

## Storage Classes

Canonical source is stored in `src/`, `include/`, `tb/`, `rtl_tb/`, and `scripts/`. The generated canonical merged source is `src/boom_core_merged.cpp`; `scripts/generate_merged.sh` is its canonical generator. `src/boom_all.cpp` is a long-lived non-canonical dirty file: never modify, clean, commit, or use it for a canonical build.

Final reviewable evidence belongs under `reports/<gate>/`. Keep summaries, manifests, Markdown, CSV, JSON, resource and timing reports, verification matrices, selected logs, and evidence explicitly referenced by an accepted report. Never remove the `reports/` root.

Temporary HLS, Vivado, XSim, native, and program-build work belongs under `${BOOM_BUILD_ROOT:-/tmp/boom_hls}/<gate>/<stage>/<top>.<unique>/`. Do not create HLS projects, XSim databases, copied source trees, binaries, or wave databases under `reports/`. Copy only small final evidence into the report tree.

## Lifecycle

Generated HLS projects, `.autopilot` databases, `.Xil`, `xsim.dir`, temporary binaries, object files, copied generated RTL, and transient waveforms must be removed after evidence extraction. A passing Gate run should delete its large temporary workspace by default. A failed run should preserve small logs and any specifically needed failing waveform, not an unlimited full tool project.

`BOOM_KEEP_BUILD=1` may be used for an explicit debugging run. The default is `BOOM_KEEP_BUILD=0`; scripts should clean large workspaces after success and should avoid retaining them after failure unless the operator explicitly opted in.

## Maintenance

Run `bash scripts/maintenance/cleanup_workspace.sh` for a dry-run inventory. Review every `SAFE_DELETE` path. Run `bash scripts/maintenance/cleanup_workspace.sh --execute` only when those exact generated paths are no longer active. Use `--older-than-days N` and `--build-root PATH` to narrow the scan.

Run `bash scripts/maintenance/check_workspace_size.sh` at Gate start and end. The defaults are a non-failing warning at 20 GiB and a severe warning at 50 GiB. Override them with `WORKSPACE_SIZE_WARNING_GB` and `WORKSPACE_SIZE_SEVERE_GB`. Gate wrappers should print `WORKSPACE_SIZE_BEFORE` and `WORKSPACE_SIZE_AFTER` where lifecycle integration is available.

Never use `git clean -fd`, `git clean -fdx`, broad wildcard removal, `git reset --hard`, checkout/restore of the worktree, or automatic stash as workspace cleanup. Always protect tracked files, uncommitted source, accepted evidence, active tool workspaces, and `src/boom_all.cpp`.

## Gate Script Rule

New Gate scripts must source `scripts/common/gate_workspace.sh`, use one owned workspace, install an exit cleanup trap, and honor `BOOM_KEEP_BUILD`. They must place only curated evidence in `reports/<gate>/`. Existing scripts that predate this policy should be migrated when next maintained without changing Gate functionality.
