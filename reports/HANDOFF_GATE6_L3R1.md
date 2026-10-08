# BOOM HLS Gate6 L3R1 Handoff

Date: 2026-10-08

Prepared from workspace: `/home/lab_726/boom/hls_boom`

Release baseline commit: `2c6c5067274d9c310c637d20752bc271590bcded`

Pre-handoff working branch: `gate3.8-rtl-verification`

Handoff branch target: `handoff/gate6-l3r1-20261008`

## Project Goal

This repository implements a Vitis HLS synthesizable C++ model of a SmallBoom-style out-of-order RV64 core. The project incrementally reconstructs and verifies front-end, issue/execute, completion, branch recovery, RV64M, RVC, predictor/FTQ, and LSU behavior through gated native, CSim, generated RTL, and synthesis evidence.

The current engineering objective is Gate6.0 L3: extend the LSU from L2 conservative store-to-load forwarding to multiple external outstanding loads while preserving the existing no-speculation/no-replay memory-ordering contract.

## Current Architecture State

| Area | State |
| --- | --- |
| Frontend/RVC/fetch buffer | Implemented through Gate5.3/Gate5.4 product integration, but current RVC baseline provenance is blocked. |
| Predictor/FTQ | Product BIM/FTQ supported subset integrated through Gate5.4 PF5. TAGE/BTB/RAS/full ICache remain out of scope. |
| Issue/execute/completion | W3/W4-style fixed MEM/INT lanes and retained completion slots are implemented for the supported subset. |
| RV64M | Gate4.1 M3C accepted historically for supported multiply/divide scope. |
| LSU L2 | Conservative forwarding contract in baseline release: youngest older overlapping single-store full-cover only; no speculation, violation detection, replay, or merge. |
| LSU L3 | Current source adds per-LQ outstanding external load ownership with bounded txid allocation and out-of-order response matching. |

L3 architectural contract:

| Field | Value |
| --- | --- |
| `L3_OUTSTANDING_ARCHITECTURE` | `PER_LQ_ENTRY_OUTSTANDING_OWNERSHIP` |
| `CONFIG_4_MAX_OUTSTANDING_LOADS` | `4` |
| `CONFIG_8_MAX_OUTSTANDING_LOADS` | `8` |
| `CONFIG_16_MAX_OUTSTANDING_LOADS` | `16` |
| `ACTIVE_TXID_UNIQUENESS` | `true` |
| `OUT_OF_ORDER_RESPONSE_SUPPORT` | `true` |
| `DUPLICATE_RESPONSE_REJECTED` | `true` |
| `FORWARDED_LOAD_CONSUMES_EXTERNAL_SLOT` | `false` |
| memory-order speculation | `false` |
| memory-order violation detection | `false` |
| load replay | `false` |

## Repository Layout

| Path | Purpose |
| --- | --- |
| `include/` | Canonical headers and persistent state/type definitions. |
| `src/` | Canonical modular product C++ implementation plus generated merged source. |
| `src/boom_core_merged.cpp` | Canonical generated merged HLS translation unit from `scripts/generate_merged.sh`. |
| `src/boom_all.cpp` | Protected legacy monolithic snapshot; current SHA256 must remain `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`; do not normalize or restore casually. |
| `tb/differential/` | Native and differential C++ test sources. |
| `tb/programs/` | Assembly/linker program sources; generated binaries should stay out of the repo. |
| `rtl_tb/` | Handwritten SystemVerilog RTL testbench sources. |
| `scripts/` | Gate runners, HLS Tcl, XSim, synthesis, and maintenance scripts. |
| `docs/` | Architecture, policy, and implementation notes. |
| `reports/` | Curated evidence and handoff artifacts. |

## HLS C++ To RTL Flow

1. Edit modular source under `include/` and `src/`.
2. Regenerate the merged HLS source with `scripts/generate_merged.sh` when product source changes.
3. Native tests compile modular source directly with `g++` and do not require Vitis.
4. Vitis HLS CSim flows use Tcl wrappers such as `scripts/gate6_0/l3_multi_outstanding_csim.tcl`.
5. Vitis HLS synthesis flows consume `src/boom_core_merged.cpp` and top wrappers, generally through scripts under `scripts/` and gate-specific Tcl.
6. Generated RTL/XSim flows use existing gate runners and `rtl_tb/` harnesses; Gate6 L3 focused RTL and full-core RTL are not yet run.
7. Large build outputs belong under `/home/lab_726/opencode_tmp` or another external build root, not in the repository.

Important command examples:

```sh
bash scripts/generate_merged.sh
bash scripts/gate6_0/run_l3_multi_outstanding_native.sh
bash scripts/gate6_0/run_l3_multi_outstanding_csim.sh
```

## Key Source SHA256

| Path | SHA256 |
| --- | --- |
| `include/boom_state.hpp` | `8f557a3f6fac086117bedcfb8d5a5cd17effd1659e56f9419504f3b2318cf432` |
| `include/boom_types.hpp` | `572bb455887499c3a7a07af0f8f77fff8213feccff0b116fd1781e416d03d51c` |
| `include/boom_config.hpp` | `344fb3801d79ddc9e99de324496440f933cc7d0614985a3ceb10039356b012eb` |
| `src/lsu.cpp` | `9da26198833c1fc677316f318edc29f531188c0aac6c6a293e5d98623ac1e01b` |
| `src/completion.cpp` | `1142cb5c943b43b55b6fc4bcb05974a3bc39b2df09f34f210397e9d42f5189bc` |
| `src/branch.cpp` | `298bd63af96f4f0c7af0d3207a313b5c6b293fdb14976bf7c864474bbf917a6c` |
| `src/reset.cpp` | `790945f1099751e0bbb087163ddda7e92d71627e4f8ac2ca7088796ce85ba45d` |
| `src/commit.cpp` | `f35cf98984224398a51ab4dc064060df818862e6e7ca2a2dfc6ea2176b34e1b7` |
| `src/boom_core_merged.cpp` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` |
| `src/boom_all.cpp` | `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c` |
| `tb/differential/g6_l3_multi_outstanding_tests.cpp` | `2bb957658b464a51ebb9cb41ab9443d156645235e52cff6b0d0ec15cebf98316` |
| `scripts/gate6_0/run_l3_multi_outstanding_native.sh` | `69c5530a110caa806393494ce312b5c3a84400c6450eb4b7fd865390e8d4da6d` |
| `scripts/gate6_0/run_l3_multi_outstanding_csim.sh` | `7d4387467fab223716cd33c54ed3bc929cc9ca959cbcb4ced2199bde924ce518` |
| `scripts/gate6_0/l3_multi_outstanding_csim.tcl` | `e44f27d77939f02ca07fc9d5d8e7b9ae5e88887761f73b9bda2fc2f470bc0c13` |
| `tb/differential/rvc_fetch_tests.cpp` | `f5f0238106be13b14c35a2077a69a6c8de5f6eba65eb5be980fe467f148d0b6b` |
| `scripts/gate5_2/run_r2_native_tests.sh` | `ac3c72c50ee26537aa26ce8bc2327d9cdc152a6b7c07afb802f328e20a108c53` |

Full source hash inventory is in `reports/gate6_0_full_lsu/l3/handoff_audit/key_source_sha256.txt`.

## Gate Acceptance Status

### Current Gate6 L3 Work Evidence

These are current Gate6 L3 work artifacts already present in this workspace. They were not rerun as part of the final handoff commit unless noted.

| Test or matrix | Status | Evidence class | Evidence path |
| --- | --- | --- | --- |
| L3 native five-config `4_4`, `8_8`, `16_16`, `4_16`, `16_4` | PASS | Current Gate6 L3 run, reused in handoff | `/home/lab_726/opencode_tmp/g6_l3/native/*.log` and `reports/gate6_0_full_lsu/l3/l3_results.md` |
| L3 CSim five-config | PASS | Current Gate6 L3 run, reused in handoff | `/home/lab_726/opencode_tmp/g6_l3/csim/*.log` |
| L3 directed | PASS | Current Gate6 L3 run, reused in handoff | `reports/gate6_0_full_lsu/l3/directed_test_matrix.csv` |
| L3 bounded exhaustive | PASS, 64 states | Current Gate6 L3 run, reused in handoff | `reports/gate6_0_full_lsu/l3/exhaustive_summary.csv` |
| L3 random default `8_8` | PASS, 256 x 8192 | Current Gate6 L3 run, reused in handoff | `reports/gate6_0_full_lsu/l3/random_summary.csv` |
| L2 native preservation five-config | PASS | Current Gate6 L3 preservation run, reused in handoff | `/home/lab_726/opencode_tmp/g6_l3/l2_native/*.log` |
| L2 CSim preservation five-config | PASS | Current Gate6 L3 preservation run, reused in handoff | `/home/lab_726/opencode_tmp/g6_l3/l2_csim/*.log` |
| Current-source `rvc_fetch_tests` | FAIL, 1253 failures | Rerun during L3R1/handoff | `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_current.*` |
| Clean release archive `rvc_fetch_tests` | FAIL, 1253 failures | Rerun during L3R1/handoff | `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_baseline.*` |

### Historical PASS Evidence Not Newly Rerun

| Area | Historical status | Notes |
| --- | --- | --- |
| Gate1 to Gate5.4 implementation status | Multiple PASS reports | See `docs/implementation_status.md`; do not treat as newly rerun during this handoff. |
| Gate5.2 RVC release report | Historical PASS | The checked-in `rvc_fetch_tests.log` says PASS, but its own source-hash provenance does not match the release commit source. |
| Gate5.4 PF5/PF6 reports | Historical PASS where recorded | Not rerun during L3R1. |
| W3/W4/M3 preservation | Historical PASS where recorded | Not rerun during L3R1 after RVC baseline control failed. |

### Not Yet Executed For Gate6 L3 Closure

| Item | Status |
| --- | --- |
| Gate6 L3 focused RTL | NOT_RUN |
| Gate6 L3 full-core synthesis/PPA | NOT_RUN |
| Gate6 L3 full RTL | NOT_RUN |
| PF6 final preservation after L3 | NOT_RUN |
| W3/W4/M3 final preservation after L3 | NOT_RUN |
| Post-route timing/PPA | NOT_RUN |
| Official Chipyard or full BOOM cycle equivalence | NOT_RUN/BLOCKED by known environment limits |

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false` until the RVC provenance issue and the remaining RTL/synthesis/PPA/preservation gates are closed.

## RVC Baseline Provenance Issue

Current blocker: `rvc_fetch_tests` fails with 1253 failures in both current source and clean release archive source.

The clean archive was extracted from `2c6c5067274d9c310c637d20752bc271590bcded` using `git archive`. Current and clean archive reruns produce identical stdout and stderr hashes:

| Artifact | SHA256 |
| --- | --- |
| current stdout | `b7cb21e31c54f9f7cacae344d0d2dcd6ab0f87ded3b93813dce31d31753cdf2b` |
| clean archive stdout | `b7cb21e31c54f9f7cacae344d0d2dcd6ab0f87ded3b93813dce31d31753cdf2b` |
| current stderr | `9833e480cb60ca32ea9ca0588994e1c18d3246fc10f16e6e6fe0e031d4c55337` |
| clean archive stderr | `9833e480cb60ca32ea9ca0588994e1c18d3246fc10f16e6e6fe0e031d4c55337` |

First failing assertion in both reruns:

| Field | Value |
| --- | --- |
| test | `C/C` |
| assertion | `FAIL[9]` |
| expected | no request outside the constructed one-word image |
| actual | request at `0x20004` |

The release PASS artifact is internally inconsistent with the source tree. `reports/gate5_2_rvc/r2/source_hashes_after.txt` records these historical source hashes:

| File | Historical PASS hash | Clean release/current hash |
| --- | --- | --- |
| `tb/differential/rvc_fetch_tests.cpp` | `5d134a4c430f6fd049954a5179e156df4744551d637cacfdcd37a5178b52ff2c` | `f5f0238106be13b14c35a2077a69a6c8de5f6eba65eb5be980fe467f148d0b6b` |
| `scripts/gate5_2/run_r2_native_tests.sh` | `2bcb5194927677e8e0dda21cc61c413656de472678f7a023bcba4f125c67e65c` | `ac3c72c50ee26537aa26ce8bc2327d9cdc152a6b7c07afb802f328e20a108c53` |

Current best assessment: this is a baseline provenance mismatch, not an L3 product regression. Do not change product code to make `rvc_fetch_tests` pass until the RVC historical source/runner mismatch is resolved.

Detailed RVC evidence:

| File | Purpose |
| --- | --- |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/failure_inventory.md` | Failure inventory and first failure. |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/failure_groups.csv` | Failure grouping into 20 test areas and 71 signatures. |
| `reports/gate6_0_full_lsu/l3/rvc_regression_triage/baseline_comparison.md` | Current vs clean archive comparison and hash mismatch. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_provenance_commands.md` | Exact commands and provenance details. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_current.stderr.log` | Current detailed 1253 FAIL lines. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_baseline.stderr.log` | Clean archive detailed 1253 FAIL lines. |

## Minimal Reproduction

Current-source RVC focused failure:

```sh
mkdir -p /home/lab_726/opencode_tmp/g6_l3_rvc_current/build /home/lab_726/opencode_tmp/g6_l3_rvc_current/report/logs
g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas -I/home/lab_726/boom/hls_boom/include \
  /home/lab_726/boom/hls_boom/tb/differential/rvc_fetch_tests.cpp \
  /home/lab_726/boom/hls_boom/src/frontend.cpp \
  /home/lab_726/boom/hls_boom/src/fetch_packet.cpp \
  /home/lab_726/boom/hls_boom/src/fetch_buffer.cpp \
  /home/lab_726/boom/hls_boom/src/predecode.cpp \
  /home/lab_726/boom/hls_boom/src/predictor.cpp \
  /home/lab_726/boom/hls_boom/src/rvc.cpp \
  /home/lab_726/boom/hls_boom/src/decode.cpp \
  /home/lab_726/boom/hls_boom/src/branch.cpp \
  /home/lab_726/boom/hls_boom/src/divider.cpp \
  -o /home/lab_726/opencode_tmp/g6_l3_rvc_current/build/rvc_fetch_tests \
  2>/home/lab_726/opencode_tmp/g6_l3_rvc_current/report/logs/rvc_fetch_tests_compile.log
/home/lab_726/opencode_tmp/g6_l3_rvc_current/build/rvc_fetch_tests
```

Clean release baseline control:

```sh
mkdir -p /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/build /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/report/logs
git archive 2c6c5067274d9c310c637d20752bc271590bcded | tar -x -C /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src
g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas -I/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/include \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/tb/differential/rvc_fetch_tests.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/frontend.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/fetch_packet.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/fetch_buffer.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/predecode.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/predictor.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/rvc.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/decode.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/branch.cpp \
  /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/src/divider.cpp \
  -o /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/build/rvc_fetch_tests \
  2>/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/report/logs/rvc_fetch_tests_compile.log
/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/build/rvc_fetch_tests
```

L3 native matrix rerun when RVC provenance is resolved:

```sh
BOOM_BUILD_ROOT=/home/lab_726/opencode_tmp/g6_l3/native_rerun bash scripts/gate6_0/run_l3_multi_outstanding_native.sh
```

L3 CSim matrix rerun when RVC provenance is resolved:

```sh
BOOM_BUILD_ROOT=/home/lab_726/opencode_tmp/g6_l3/csim_rerun bash scripts/gate6_0/run_l3_multi_outstanding_csim.sh
```

## Suggested Next Work

1. Resolve RVC baseline provenance by finding the historical `rvc_fetch_tests.cpp` hash `5d134...` and runner hash `2bcb...`, or regenerate the Gate5.2 R2 PASS/FAIL evidence from source actually present at `2c6c506...`.
2. Once RVC baseline provenance is resolved, decide whether `rvc_fetch_tests` failure is expected baseline drift, a test update that lacks product support, or a real product regression.
3. If product code changes are made, regenerate `src/boom_core_merged.cpp` and rerun L3 native five-config, L3 CSim five-config, L2 native/CSim preservation, and the required directed/exhaustive/random set.
4. Do not run Gate6 L3 focused RTL, full synthesis/PPA, PF6, W3, W4, or M3 final preservation until the RVC blocker is closed.
5. After RVC is closed, proceed with Gate6 L3 RTL and synthesis closure, then final preservation.

## Handoff Audit Files

| Path | Purpose |
| --- | --- |
| `reports/gate6_0_full_lsu/l3/handoff_audit/git_audit.txt` | HEAD, branch, remote, index, protected hash, recent commits. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/git_status_short.txt` | Full dirty worktree inventory at handoff prep time. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/l3_source_diff_name_status.txt` | Reviewed L3 source/test/script diff scope. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/key_source_sha256.txt` | SHA256 of tracked key source/script/test files. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_provenance_hashes_and_versions.txt` | Tool versions, binary/log hashes, and RVC source hashes. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_release_pass_provenance_excerpt.txt` | PASS artifact and source-hash mismatch excerpt. |
| `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_release_pass_log_git_history.txt` | Git history for historical PASS log. |

## Handoff Constraints Honored

| Constraint | Status |
| --- | --- |
| Product code modified during handoff | no |
| `src/boom_all.cpp` hash preserved | yes, `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c` |
| `git reset --hard` / `git clean -fd` / force push | not used |
| RVC product repair attempted | no |
| Historical PASS reclassified as newly rerun | no |
