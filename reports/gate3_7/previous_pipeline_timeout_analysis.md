# Previous CORE_CYCLE Pipeline Timeout Analysis

## Verdict

Exactly one directly evidenced `BOOM_HLS_ENABLE_CORE_PIPELINE=1` synthesis attempt exists. Gate 3.2 requested `PIPELINE II=1` on `boom_core_top/CORE_CYCLE` and timed out after the repository-recorded 15-minute limit during Presyn 2 transformations. It did not reach scheduling, achieved-II calculation, binding, RTL generation, or csynth report generation.

## Attempt Configuration

| Item | Value |
|---|---|
| Tool | Vitis HLS 2021.2 build 3367213 |
| Project | `boom_hls_gate3_2_performance` |
| Solution | `solution_performance` |
| Top | `boom_core_top` |
| Part | `xczu7ev-ffvc1156-2-e` |
| Clock | 10 ns |
| Compile define | `-DBOOM_HLS_ENABLE_CORE_PIPELINE=1` |
| Source directive | `#pragma HLS PIPELINE II=1` on `CORE_CYCLE` |
| Reset | retained |
| Automatic loop pipeline | disabled by `config_compile -pipeline_loops 0` |
| Manual partition/unroll/dependence directives | none |

The attempt-time preprocessed source and pragma database prove that the II=1 source pragma was active. The solution database contains only the top selection; no Tcl pipeline, partition, unroll, or false-dependence directive was applied.

## Last Completed Pass

Named top-level phases completed:

| Phase | Elapsed | Current Allocated Memory |
|---|---:|---:|
| file checks | 0.01 s | 1.321 GB |
| source analysis/preprocessing | 0.40 s | 1.323 GB |
| compiling optimization and transform | 36.51 s | 464.461 MB |
| checking pragmas | 0.01 s | 464.461 MB |
| standard transforms | 1.67 s | 497.691 MB |
| checking synthesizability | 1.45 s | 500.129 MB |

The internal flow then completed Presyn 1 and started Presyn 2. The final observable messages perform if-conversion in `rob_commit_module` and `older_store_in_rob`; Presyn 2 has no completion record.

Classification: `TIMEOUT_DURING_PRESYN2_TRANSFORMATION`.

The final if-conversion message identifies the last visible operation, not a proven single root cause.

## Transformation Evidence

| Event | Count/Result |
|---|---:|
| loops marked complete-unroll due outer pipeline | 28 |
| completed unroll messages | 25 |
| variable-bound loops not completely unrolled | 3 |
| automatic partition records | 0 |
| high-level and stream/pack inline records | 73 |
| memory-promotion result records | 0 |
| internal default-depth stream deadlock warnings | 5 |

The log says all loops in `issue_module` and `execute_module` are being unrolled for pipelining. Factors include ROB-depth 32, branch/register loops, issue-depth 8, and smaller lane loops. This transformation expansion occurs before scheduler dependency diagnostics.

## Missing Evidence

The run produced no:

- `*_csynth.rpt`
- achieved II
- schedule or binding result
- dependence violation warning
- memory-port conflict warning
- resource-conflict result
- generated RTL
- final Vitis runtime/peak memory line

The `.time` file is empty. The 15-minute duration comes from Gate 3.2 structured status; true OS peak RSS was not captured. The highest HLS current-allocation message is 1.323 GB and must not be reported as measured peak RSS.

## Earlier Pre-Refactor Timeout

The separate Gate 3.1C/early Gate 3.2 run timed out after 30 minutes with a hardcoded pipeline and a per-cycle `next_state` whole-state copy. It generated 427 partition records, including 395 `next_state.*` temporaries. Gate 3.2 removed that copy before the macro-enabled 15-minute attempt.

The two failures must not be conflated:

- pre-refactor: large whole-state-copy partition expansion;
- post-refactor macro II=1: zero recorded partitions, extensive implied unroll/inlining, timeout in Presyn 2 before scheduling.

## Gate 3.7 Consequence

P1 removes the requested II and lets Vitis choose. It uses a solution-local directive rather than the hardcoded macro, retains reset, and records `/usr/bin/time` output. P2-P6 may run only if P1 produces a valid report, because the previous attempt contains no scheduler-derived II bound.

Frozen copies of the prior raw log, time file, Tcl/directives, internal flow log, preprocessed source, pragma dump, structured status, and pre-refactor partition evidence are stored in `reports/gate3_7/previous_pipeline_artifacts/`.
