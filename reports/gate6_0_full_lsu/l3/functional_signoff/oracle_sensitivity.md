# Oracle Sensitivity

The sensitivity checks use isolated copies of observed PPA6 traces under `/home/lab_726/opencode_tmp`; product RTL, product source, and the committed PPA6 PASS trace are not modified.

| Case | Expected Validator Status | Observed Validator Status | Detected Failures |
| --- | --- | --- | --- |
| `baseline` | `PASS` | `PASS` | `NONE` |
| `swap_load_return_data` | `FAIL` | `FAIL` | `txid_owner_data_match` |
| `omit_required_response` | `FAIL` | `FAIL` | `all_responses_present;txid_owner_data_match` |
| `tamper_expected_register_x10` | `FAIL` | `FAIL` | `final_registers_match` |

## Result

`L3_R6_ORACLE_SENSITIVITY=PASS`

`L3_R6_ORACLE_INDEPENDENCE=PASS_STATIC_PROGRAM_ORACLE_NO_DUT_STATE`
