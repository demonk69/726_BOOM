# Implementation Blockers

## Status

| Field | Value |
|---|---|
| `L3_PPA1_EVIDENCE_COMMIT` | `236733e80a1fd60851436c5b9fe066ff81b09ea7` |
| `L3_PHYS_IO_PORTS_REQUIRED` | 1940 |
| `L3_PHYS_IO_PORTS_AVAILABLE` | 719 |
| `L3_PRODUCT_CODE_MODIFIED` | `false` |
| `G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED` | `false` |
| `PPA_GATE_STATUS` | `UNRESOLVED` |

## Blockers

| Severity | Blocker | Evidence | Required Closure |
|---|---|---|---|
| High | Raw HLS top cannot be used as package top | Vivado `Place 30-415`: 1940 I/O ports required, 719 available user I/O | Add physical wrapper/platform integration that internalizes wide streams and debug outputs |
| High | 6.450 ns physical timing is not met | Routed OOC checkpoint re-analyzed at 6.450 ns: WNS -2.835 ns, TNS -2065.701 ns, 3410 setup failing endpoints | Timing-closure work after wrapper exists; investigate ROB commit route-dominated enable fanout |
| Medium | No full-system post-route result exists | Raw top fails before placement; OOC result excludes package-level wrapper and real board I/O | Implement real top-level wrapper and run full place/route with board constraints |
| Medium | OOC DRC warnings remain | 45 route DRC warnings: DSP pipelining plus OOC no-routable-load warning | Review DSP pipelining if timing remains limited; re-evaluate RTSTAT-10 in real wrapper context |
| Medium | L3 acceptance gate remains policy-unresolved | PPA1 reports use review threshold and do not promote it to final acceptance gate | Define final PPA acceptance threshold and timing target |

## Non-Blockers Established By PPA2

| Item | Result |
|---|---|
| Fabric routability without package I/O | PASS for OOC `CONFIG_4_4` |
| Route errors | 0 |
| Congestion above level 5 | None reported after route |
| OOC package I/O buffers | 0 `IBUF`, 0 `INBUF`, 0 `OBUF`, 0 `IBUFCTRL` |
| Product source pollution | No product `src/` changes made for PPA2 |

## Carry-Forward Outstanding Evidence

The outstanding-load evidence remains valid as behavioral evidence, but it is not a physical implementation closure item by itself.

| Config | Max observed outstanding | Evidence |
|---|---:|---|
| `4_4` | 4 | `reports/gate6_0_full_lsu/l3/rtl_verification/transaction_ownership_trace.csv:5` |
| `8_8` | 8 | `reports/gate6_0_full_lsu/l3/rtl_verification/transaction_ownership_trace.csv:55` |
| `16_16` | 16 | `reports/gate6_0_full_lsu/l3/rtl_verification/transaction_ownership_trace.csv:121` |
| `4_16` | 4 | `reports/gate6_0_full_lsu/l3/rtl_verification/transaction_ownership_trace.csv:191` |
| `16_4` | 16 | `reports/gate6_0_full_lsu/l3/rtl_verification/transaction_ownership_trace.csv:249` |

The count is simultaneous in-flight loads, not total load-request count.

## Next Required Work

1. Create a real physical wrapper/integration top with package-valid I/O.
2. Run synthesis, placement, route, DRC, congestion, and timing for that wrapper.
3. Analyze the 6.450 ns critical path after wrapper integration.
4. Decide whether commit trace/debug counters are production interfaces or debug-only features.
5. Finalize the PPA acceptance policy before marking the gate passed.
