# PF3 Workspace Hygiene Check

`scripts/maintenance/check_workspace_size.sh` reports:

```text
REPOSITORY_TOTAL_BYTES=2695532544
REPOSITORY_TOTAL=2.6GB
BUILD_TOTAL_BYTES=1952505856
BUILD_TOTAL=1.9GB
REPORTS_TOTAL_BYTES=2468724736
REPORTS_TOTAL=2.3GB
WORKSPACE_SIZE_WARNING=NONE
```

PF3 build and temporary execution used `/tmp/boom_hls`. The accidental local
focused-csynth project from the first run was removed. Historical report/build
content and unrelated dirty files were not altered. `src/boom_all.cpp` remains
the pre-existing dirty legacy snapshot, is absent from active build manifests,
and has current SHA-256
`d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.

```text
REPOSITORY_HYGIENE_PRESERVED=true
SRC_BOOM_ALL_EXCLUDED=true
```
