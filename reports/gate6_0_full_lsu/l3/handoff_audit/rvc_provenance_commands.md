# RVC Provenance Commands

## Current Source Focused Build

Working directory: `/home/lab_726/boom/hls_boom`

Compile command used for focused current reproduction:

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
```

Run command used for committed handoff logs:

```sh
/home/lab_726/opencode_tmp/g6_l3_rvc_current/build/rvc_fetch_tests \
  > reports/gate6_0_full_lsu/l3/handoff_audit/rvc_current.stdout.log \
  2> reports/gate6_0_full_lsu/l3/handoff_audit/rvc_current.stderr.log
```

Exit code file: `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_current.exit_code`

## Clean Release Archive Focused Build

Source extraction:

```sh
mkdir -p /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/build /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/report/logs
git archive 2c6c5067274d9c310c637d20752bc271590bcded | tar -x -C /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src
```

Compile command used for clean archive reproduction:

```sh
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
```

Run command used for committed handoff logs:

```sh
/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/build/rvc_fetch_tests \
  > reports/gate6_0_full_lsu/l3/handoff_audit/rvc_baseline.stdout.log \
  2> reports/gate6_0_full_lsu/l3/handoff_audit/rvc_baseline.stderr.log
```

Exit code file: `reports/gate6_0_full_lsu/l3/handoff_audit/rvc_baseline.exit_code`

## Archived Runner Confirmation

The archived runner was also run from the clean archive:

```sh
BOOM_R2_BUILD_DIR=/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/runner_build \
BOOM_R2_REPORT_DIR=/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/runner_report \
  bash /home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/scripts/gate5_2/run_r2_native_tests.sh
```

It exited `1` after `rvc_fetch_tests` and produced `RVC_FETCH_METRICS assertions=4389 cases=414 packets=104 requests=430 protected=288 failures=1253`.

## Historical PASS Provenance

The release commit contains `reports/gate5_2_rvc/r2/logs/rvc_fetch_tests.log` with PASS metrics and `RVC_FETCH_CANONICAL_CPP11_UNIQUE_PASS_726`.

The same release report set records these source hashes in `reports/gate5_2_rvc/r2/source_hashes_after.txt`:

```text
5d134a4c430f6fd049954a5179e156df4744551d637cacfdcd37a5178b52ff2c  tb/differential/rvc_fetch_tests.cpp
2bcb5194927677e8e0dda21cc61c413656de472678f7a023bcba4f125c67e65c  scripts/gate5_2/run_r2_native_tests.sh
```

The clean release archive now contains these different hashes:

```text
f5f0238106be13b14c35a2077a69a6c8de5f6eba65eb5be980fe467f148d0b6b  tb/differential/rvc_fetch_tests.cpp
ac3c72c50ee26537aa26ce8bc2327d9cdc152a6b7c07afb802f328e20a108c53  scripts/gate5_2/run_r2_native_tests.sh
```

This source/runner hash mismatch is the current best explanation for why the historical PASS artifact is not reproducible from the clean release archive.
