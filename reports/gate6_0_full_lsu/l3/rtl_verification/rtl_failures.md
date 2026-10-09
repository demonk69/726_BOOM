# L3R2 RTL Failures

No unresolved CONFIG_4_4 RTL failures remain.

Final RTL status: PASS

Final evidence: `logs/config_4_4_simulation.log`

Notes:

The RTL harness required generated-field alignment for the HLS-packed `MicroOp.queue` fields. This was a testbench seed-map issue only; no product source or generated RTL was modified.

Extended configs were not run in RTL and are therefore unclassified rather than failed.
