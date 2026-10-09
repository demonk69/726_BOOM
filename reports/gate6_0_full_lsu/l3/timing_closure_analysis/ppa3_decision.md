# PPA3 Decision

## Decision

PPA3 improves the physical timing evidence: a fresh 6.450 ns OOC implementation of `CONFIG_4_4` places, routes, and meets timing. The full Gate6.0 L3 PPA gate remains unresolved because the deployable package-valid system wrapper has not been implemented or routed.

## Required Fields

| Field | Value |
|---|---|
| `L3_PPA2_EVIDENCE_COMMIT` | `cf4d4ab59f3150896ecd8fd2b6a2af2f87c732bc` |
| `L3_PPA3_CONSTRAINTS_VALID` | `OOC_INTERNAL_PASS_SYSTEM_IO_PENDING` |
| `L3_PPA3_CRITICAL_PATH_MODULE` | `grp_boom_core_cycle_io_fu_6197/grp_rob_commit_module_fu_3904` |
| `L3_PPA3_CRITICAL_PATH_CLASS` | `ROB_COMMIT` |
| `L3_PPA3_MAX_LOGIC_LEVELS` | `6` |
| `L3_PPA3_ROUTING_DELAY_DOMINANT` | `true` |
| `L3_PPA3_FRESH_6P450_PLACE` | `PASS` |
| `L3_PPA3_FRESH_6P450_ROUTE` | `PASS` |
| `L3_PPA3_FRESH_6P450_WNS` | `0.086` |
| `L3_PPA3_FRESH_6P450_TNS` | `0.000` |
| `L3_PPA3_FRESH_6P450_FAILING_ENDPOINTS` | `0` |
| `L3_PPA3_SYSTEM_IO_STRATEGY` | `REQUIRES_PACKAGE_VALID_WRAPPER_WITH_INTERNAL_AXI_OR_MEMORY_FABRIC` |
| `L3_PPA3_PRODUCT_CODE_MODIFIED` | `false` |
| `PPA_GATE_STATUS` | `UNRESOLVED` |
| `G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED` | `false` |

## Evidence Chain

| Item | Result | Evidence |
|---|---|---|
| PPA2 OOC route at 10 ns | PASS | `reports/gate6_0_full_lsu/l3/physical_implementation/ooc_implementation.md` |
| PPA2 old route re-analyzed at 6.450 ns | FAIL | WNS `-2.835 ns`, TNS `-2065.701 ns`, failing endpoints `3410` |
| PPA3 constraint audit | OOC internal PASS, system I/O pending | `constraint_audit.md` |
| PPA3 critical path extraction | Route-dominated ROB commit CE/control path | `critical_path_groups.md`, `critical_paths.csv` |
| PPA3 fresh 6.450 ns implementation | PASS | `fresh_6p450_implementation.md` |
| System top I/O feasibility | Wrapper required | `io_boundary_strategy.md` |

## Artifact Provenance

| Artifact | SHA-256 |
|---|---|
| RTL manifest | `b50e2d8fe57cc9813468ff52ce07c0919cd0a64c33645e9121dad1cf3c18fdeb` |
| PPA2 post-synth netlist DCP used as PPA3 input | `94c62ad0095256822985f92e2ec03acd6ff9ce47ffb2b67e69e695b3578ae5b2` |
| OOC clock XDC | `44454f3b63f252f61cf5cf536e77fb6cdfe8e5147ec993e8f18e68b3550b0507` |
| PPA3 fresh routed DCP | `7adb1fb683743830c0e1e97c6c98fd5ba6d92c33fe1735c6f36f6d0667d73377` |
| PPA3 fresh post-route timing summary | `5c848ceaf2c07264ab1f20b18db564735030bf33264ea84898158f1c218f8eb5` |

## Why The Gate Stays Unresolved

The fresh OOC implementation proves that the core fabric can meet the 6.450 ns target under OOC conditions when implementation is run with the correct target clock from the start. It does not prove that the final system can be packaged and timed because:

1. The raw HLS top still requires `1940` package-level I/O ports, which exceeds the package budget.
2. The OOC implementation uses `0` bonded IOBs, so it intentionally excludes the package boundary.
3. Board-level input/output delays and external interface timing cannot be finalized until a real wrapper/top exists.
4. Only `CONFIG_4_4` has fresh 6.450 ns OOC physical evidence in PPA3.
5. No product `src/` changes were made to implement a wrapper or alter the interface.

## Next Required Closure

1. Implement a package-valid wrapper that internalizes IMEM, DMEM, and commit trace streams into fabric interfaces.
2. Preserve AXIS backpressure and DMEM transaction IDs end to end.
3. Add board/platform timing constraints for the wrapper interfaces.
4. Run full-system synthesis, placement, routing, DRC, congestion, and timing at the L3 target.
5. Re-evaluate multi-config physical evidence if the acceptance gate requires more than `CONFIG_4_4`.
