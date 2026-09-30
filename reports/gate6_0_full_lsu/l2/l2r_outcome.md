# Gate 6.0 L2R Outcome

`L2R_PARAMETER_16_16_BRAM_REPAIR=PASS`

`PF6_CURRENT_SOURCE_FULL_CORE_RTL=PASS_12_OF_12`

`PF6_MANDATORY_FAULTS=PASS_2_OF_2`

`G6_0_L2_STORE_TO_LOAD_FORWARDING_VERIFIED=true`

The 16/16 configuration uses 16 BRAM18K, 3 DSP, 211984 LUT, 49382 FF, and a
6.071 ns estimated period. The accepted storage layout preserves 16 entries,
64-bit addresses and data, full older-store search, and stale-owner checks.

PF6 progress failure was classified as `HARNESS_WAIT_CONDITION_BUG`. The
testbench used generated top state 34 as a cycle observation point. In current
RTL state 34 launches reset, while state 35 is the equivalent subcall wait
state used by the previously accepted harness. Updating that observer and
synchronizing the stale generated hierarchy references restored progress and
the prior coverage counts. No product workaround was introduced.

`NEXT_REQUIRED_ACTION=COMMIT_G6_0_L2`
