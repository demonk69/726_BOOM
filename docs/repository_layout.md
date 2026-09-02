# Repository Layout

The Git worktree is `hls_boom/`, not its parent `/home/lab_726/boom`.

| Path | Purpose | Retention |
| --- | --- | --- |
| `include/` | Canonical public and internal headers | Permanent |
| `src/` | Canonical modules and approved generated merged source | Permanent |
| `tb/differential/` | Native and differential test source | Permanent |
| `tb/programs/` | Assembly/linker program source; generated binaries belong in the build root | Source permanent, binaries temporary |
| `rtl_tb/` | Handwritten generated-RTL testbench source | Permanent |
| `scripts/gate*/` | Historical Gate runners | Permanent; migrate only when maintained |
| `scripts/common/` | Build lifecycle shared by future runners | Permanent |
| `scripts/maintenance/` | Safe inventory and cleanup utilities | Permanent |
| `docs/` | Repository and design policy | Permanent |
| `reports/` | Accepted and curated evidence | Curated permanent evidence |

`src/boom_core_merged.cpp` is the canonical generated merged source where a merged translation unit is required. `src/boom_all.cpp` is protected historical dirty content and must not be modified, moved, restored, staged, or used as a canonical build input.

## Build Root

Future Vitis HLS, XSim, Vivado, native, CSim, synthesis, and program-binary output must use:

```sh
export BOOM_BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls}
```

Use `native/`, `csim/`, `csynth/`, `xsim/`, and `programs/` as logical stages, with Gate and top identifiers below them. `scripts/common/build_env.sh` creates unique directories such as `$BOOM_BUILD_ROOT/gate5_4/p2/synth_predictor_256.ABC123` and removes successful workspaces unless `BOOM_KEEP_BUILD=1`.

Do not generate large workspaces in the repository root, `src/`, `include/`, `tb/`, `rtl_tb/`, or `reports/`. Historical paths and accepted report directories remain in place to preserve runner and evidence references. Future reports should use `reports/<subsystem>/<stage>/` without renaming historical reports.
