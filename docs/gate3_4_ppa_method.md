# Gate 3.4 PPA Method

Gate 3.4 uses resource attribution before optimization. The accepted Gate 3.3 conservative baseline remains unchanged while Gate 3.4 inventories reports, generated RTL, transform logs, and targeted module csynth handles.

Rules:

- Do not enable `CORE_CYCLE` pipeline.
- Do not reduce state capacity or field width.
- Do not change branch recovery, redirect, store commit, or rollback cycle semantics.
- Treat branch recovery helper reports as overlapping, not additive.
- Prefer single-variable structure experiments before directives.

Current Gate 3.4 status: `ANALYSIS_AND_MODULE_BASELINE_COMPLETE_NO_ACCEPTED_OPTIMIZATION`.

Primary artifacts: `reports/gate3_4/resource_attribution.md`, `reports/gate3_4/module_baseline.csv`, and `reports/gate3_4/gate3_4_results.md`.
