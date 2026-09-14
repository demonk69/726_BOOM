# PF5CW Commit Whitespace and Raw Evidence Policy Repair

PF5CW changes derived evidence formatting and artifact classification only.
Canonical product source, tests, program semantics, RTL DUTs, directives,
verification results, and PPA are unchanged.

## Inventory

- Original manifest: 165 files.
- Files with whitespace findings: 14.
- Original findings: 367.
- Canonical product-source findings: 0.
- Normalized derived CSV files: 11.
- CSV logical row counts and normalized-content hashes unchanged: true.

## Policy

`PF5_FINAL_WHITESPACE_POLICY=TEXT_DIFF_CHECK_PLUS_RAW_BYTE_EXACT_EVIDENCE`

Derived CSV files were normalized from CRLF to LF without changing fields,
rows, order, values, or final verdicts. Immutable Vitis CSim logs were not
modified and are excluded from the repaired commit manifest because durable
summary, matrix, hash, and provenance records cover their results. The frozen
canonical synthesis summary remains required and byte-exact because its
accepted SHA-256 is part of the PF5 source/PPA freeze.

`TEXT_NORMALIZATION_SEMANTIC_CHANGE=false`

`CSV_LOGICAL_CONTENT_UNCHANGED=true`

`RAW_LOG_BYTES_PRESERVED=true`

`PF5CW_RAW_EVIDENCE_FILE_COUNT=3`

`PF5CW_RAW_EVIDENCE_BYTES_MODIFIED=false`

`PF5CW_PRODUCT_SOURCE_CHANGED=false`

`PF5CW_REPAIRED_STAGE_MANIFEST_COUNT=166`

`PF5_FINAL_DURABLE_ARTIFACT_COUNT=14`

`PF5_FINAL_DIRECTED_EXPECTED_CHECKS=6205`

`DIRECTED_STATUS=PASS_6205_OF_6205`

`PF2_FULL_CORE_RTL_STATUS=PASS_11_OF_11_CURRENT_SOURCE`

`W3_SOFTWARE_STATUS=PASS_400_OF_400`

`W3_FOCUSED_RTL_STATUS=PASS_11_OF_11`
