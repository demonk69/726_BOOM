# Source Diff Scope

Selected A5 product changes:

- `src/execute.cpp`: MUL-family uops consume `IssueState::issued_prs1_data` and `issued_prs2_data`, which `issue_module` resolves before setting `issued_valids`.
- `src/divider.cpp`: the `|divisor| == 1` fast path computes an equivalent result directly from the normalized dividend and divisor sign, avoiding redundant magnitude restoration.
- `src/synth_module_tops.cpp`: the direct execute synthesis harness now supplies the existing resolved-operand interface.
- `src/boom_core_merged.cpp`: generated only by `scripts/generate_merged.sh`.

Targeted harness updates:

- `tb/differential/m2b_execute_tests.cpp`
- `tb/differential/w4_bypass_tests.cpp`

No architectural state, issue arbitration, wakeup, RAW dependency, branch recovery, ROB/completion ordering, PRF contents, LSU, DCache, MMU, latency, ISA behavior, or memory behavior changed. Current 41-file product aggregate is `492cc6257137595a910273e20f77c08693146ac927b342045c1a6e71ec6284a9`; merged SHA-256 is `106a5ff727ddd3798976e80222c01f69d9359990a36afadd7566dc2241e3c4c1`.
