# Gate Artifact Policy

## CURATED_EVIDENCE

Each final Gate commit should retain only reviewable artifacts needed to establish the result:

- canonical source and headers
- test source and RTL testbench source
- runner scripts
- final Markdown report
- CSV summary or verification matrix
- artifact and source-hash manifests
- small decisive logs
- selected resource or timing summaries

An accepted final report, its manifest, and every selected artifact referenced by either are `CURATED_EVIDENCE`. Curated evidence belongs under `reports/`, is never removed by automatic cleanup, and may be duplicated when the duplication is necessary for a historical accepted path.

## REPRODUCIBLE_WORKSPACE

Vitis projects and solution directories, `.Xil`, `xsim.dir`, Vivado workspaces, program binaries, waveforms, temporary objects, caches, copied generated RTL, and duplicate console logs are `REPRODUCIBLE_WORKSPACE`. They belong under `${BOOM_BUILD_ROOT:-/tmp/boom_hls}` and should not be committed.

Successful runners clean their owned workspace. Failed runners may call `gate_preserve_failure`, or operators may set `BOOM_KEEP_BUILD=1`, when the workspace is needed for diagnosis. Cleanup must remain manifest- or build-root-bounded and must never use `git clean`, broad wildcard deletion, or remove tracked, dirty, active, unknown, or accepted-evidence paths.
