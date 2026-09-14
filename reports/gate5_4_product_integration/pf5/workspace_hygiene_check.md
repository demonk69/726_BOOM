# PF5 Workspace Hygiene Check

- Branch: `gate3.8-rtl-verification`.
- HEAD: `825df0af5889c46d7a0fa0fb77ea59e1a6a73b33`.
- Upstream ahead/behind: `0/0`.
- No commit, push, staging, restore, or checkout was performed.
- Historical `src/boom_all.cpp` changes were not edited and that file was
  excluded from PF5 builds.
- Existing unrelated dirty/untracked files were not reverted.
- PF5 full-core logs/traces are replaced per run, preventing stale PF4 evidence
  from remaining in the PF5 report directory.

`HYGIENE_VIOLATION=false`
