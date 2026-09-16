# L1R2B Case Reconciliation

```text
SOURCE_MANIFEST_FILES_CHECKED=41
SOURCE_MANIFEST_MISMATCHES=0
CANONICAL_FOCUSED_CASE_COUNT=80
CANONICAL_CASE_ID_RANGE=0_79
CANONICAL_DUPLICATE_IDS=0
CANONICAL_DUPLICATE_NAMES=0
LEGACY_17_EXACT_MAPPED=0
LEGACY_17_AMBIGUOUS=12
LEGACY_17_UNMAPPABLE=5
PILOT_EXACT_MAPPED_CASES=7
PILOT_DUPLICATE_WITH_LEGACY_CASES=0
PILOT_NEW_ACCEPTED_CASES=7
ACCEPTED_UNIQUE_CANONICAL_CASES_BEFORE_EXPANSION=7
REMAINING_CANONICAL_CASES=73
```

The old `17/17` record is aggregate-only. The 17 rows in `legacy_17_case_inventory.csv` are a forensic candidate reconstruction from an older source-incompatible Gate 3.9 suite, not the missing manifest. None may count toward canonical acceptance.

Strict pilot mapping is limited to IDs `7,14,15,23,32,34,42`. P1 asymmetric reset and P5 older-store predicate remain supporting feasibility evidence because their retained stimulus/oracle does not exactly match a default-8/8 canonical row.

The canonical catalog defines six rotating variant orders. Any `%5` variant dispatch in the abandoned command-engine testbench is invalid for exact expansion; L1R2B uses the CSV order directly.

## Expansion Outcome

```text
PATH_A_ASSIGNED_CASES=70
PATH_A_RTL_PASS_CASES=70
PATH_B_ASSIGNED_CASES=10
PATH_B_RTL_PASS_CASES=10
CANONICAL_ACCEPTED_AFTER_EXPANSION=80
CANONICAL_REMAINING_AFTER_EXPANSION=0
CANONICAL_REMAINING_IDS=NONE
FULL_CORE_PATH_B_EXIT_CODE=143
FULL_CORE_PATH_B_DURATION_SECONDS=468
FULL_CORE_PATH_B_PEAK_WORKSPACE_KIB=8955852
FULL_CORE_PATH_B_HARD_LIMIT_CROSSED=1
FULL_CORE_PATH_B_BOUNDED_RETRY_EXIT_CODE=0
FULL_CORE_PATH_B_BOUNDED_RETRY_DURATION_SECONDS=540
FULL_CORE_PATH_B_BOUNDED_RETRY_PEAK_WORKSPACE_KIB=633744
FULL_CORE_PATH_B_BOUNDED_RETRY_PEAK_RSS_KIB=6969544
FULL_CORE_PATH_B_BOUNDED_RETRY_RTL_HASH=c0165ccea43ccc41d1d2cc5514d8af17764b700b84a84af751c4b331be55fcbe
FULL_CORE_PATH_B_XSIM_PASS_CASES=10
FOCUSED_RTL_STATUS=PASS_80_OF_80
```

The six PATH_A images use fixed automatic local state, no command interpreter,
and external host/SV oracles. Their per-case results and generated RTL hashes
are recorded in `canonical_focused_rtl_acceptance.csv` and
`focused_rtl_80_resource_summary.csv`.

The first PATH_B full-core attempt stopped before RTL generation under its 8 GiB
hard limit. Audit then found that invocation omitted the accepted canonical
`config_compile -pipeline_loops 0` setting. The authorized one-time bounded
retry used the accepted synthesis flow, generated current-source default-8/8
`boom_core_top` RTL, and stayed below its 12/16 GiB workspace policy and memory
safety limits.

Each remaining canonical ID ran in a fresh XSim process. Backpressure and
ordering cases used external AXIS stalls. Reset and exception cases used
controlled precondition seeding followed by the generated full-core reset or
flush machinery. IDs 13 and 47 injected only the mismatching identity component
at the integrated generated `try_issue_load` check and used an external oracle
to require that no request reached the dmem AXIS channel. Exact stimulus,
expected requirement, actual observation, hashes, and logs are recorded in
`path_b_case_evidence.csv`.
