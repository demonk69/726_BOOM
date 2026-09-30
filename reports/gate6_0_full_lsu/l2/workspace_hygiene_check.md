# Gate 6.0 L2 Workspace Hygiene Check

- Baseline commit: `eb753342642ce6bcff2c6007283d6206c9ae3a14`.
- Branch: `gate3.8-rtl-verification`.
- Ahead/behind at commit-only audit entry: `0/0`.
- Git index at commit-only audit entry: empty.
- Active Vitis HLS/Vivado/xvlog/xelab/xsim processes: 0.
- Protected external working-tree `src/boom_all.cpp` SHA-256:
  `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- The release commit intentionally retains the baseline blob instead of this
  unrelated dirty working-tree content.
- `PRE_L2_VOLATILE_STATUS_SNAPSHOT_AVAILABLE=false`.
- `WORKSPACE_HYGIENE_EVIDENCE_MODE=PROTECTED_HASHES_PLUS_CURRENT_DIFF_CLASSIFICATION`.

The vanished `/tmp` snapshot was not reconstructed and no exact 1205-entry
comparison is claimed. Existing unrelated dirty/generated files were not
reverted or modified for cleanup. No stage, commit, push, broad restore, hard
reset, worktree creation, or L3 work was performed before the explicit final
staging audit.
