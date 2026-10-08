# RVC Provenance Reconstruction Commands

## `CMD_DB9D_HIST_RUNNER`

```sh
git worktree add --detach /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d db9d3b22fb3672751d9e012cf3130296cbf19f3a
HLS_BOOM_ROOT=/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d \
BOOM_R2_BUILD_DIR=/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d_build \
BOOM_R2_REPORT_DIR=/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d_report \
bash scripts/gate5_2/run_r2_native_tests.sh
```

## `CMD_2C6_CURRENT_S1`

This is the focused clean release/current RVC command recorded in `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_provenance_commands.md`, using product commit `2c6c5067274d9c310c637d20752bc271590bcded` for the clean release archive and the current source tree for the S1 clean HEAD rerun.

## `CMD_2C6_HIST_RUNNER`

```sh
git worktree add --detach /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed 2c6c5067274d9c310c637d20752bc271590bcded
git show db9d3b22fb3672751d9e012cf3130296cbf19f3a:tb/differential/rvc_fetch_tests.cpp > /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/tb/differential/rvc_fetch_tests.cpp
git show db9d3b22fb3672751d9e012cf3130296cbf19f3a:scripts/gate5_2/run_r2_native_tests.sh > /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/scripts/gate5_2/run_r2_native_tests.sh
HLS_BOOM_ROOT=/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed \
BOOM_R2_BUILD_DIR=/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed_build \
BOOM_R2_REPORT_DIR=/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed_report \
bash scripts/gate5_2/run_r2_native_tests.sh
```

## `CMD_2C6_HIST_TEST_CURRENT_SOURCES`

```sh
g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -I/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/include \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/tb/differential/rvc_fetch_tests.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/frontend.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/fetch_packet.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/fetch_buffer.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/predecode.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/predictor.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/rvc.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/decode.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/branch.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed/src/divider.cpp \
  -o /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed_current_compile/build/rvc_fetch_tests
/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_release_mixed_current_compile/build/rvc_fetch_tests
```

## `CMD_DB9D_CURRENT_TEST_OLD_SOURCES`

```sh
g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation -Wno-unknown-pragmas \
  -I/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d/include \
  /home/lab_726/opencode_tmp/g6_l3r1_s1_clean/tb/differential/rvc_fetch_tests.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d/src/frontend.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d/src/rvc.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d/src/decode.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d/src/branch.cpp \
  /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d/src/divider.cpp \
  -o /home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d_current_test/build/rvc_fetch_tests
/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d_current_test/build/rvc_fetch_tests
```

## `CMD_HEAD_CURRENT_RVC_S1`

This is the clean detached HEAD S1 command recorded in `reports/gate6_0_full_lsu/l3/handoff_audit/canonical_source_snapshot_audit.md` and `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_provenance_commands.md`.
