# Resource Discrepancy Analysis

Question: why did R5 CSynth estimate about `211204` LUT while compact routed PPA4 uses `49452` LUT?

## Observed Numbers

| Source | Scope | LUT Metric | Value |
| --- | --- | --- | ---: |
| R5 CSynth | HLS `boom_core_top`, pre-RTL implementation estimate | HLS estimated LUT | `211204` |
| PPA3 OOC route | Generated `boom_core_top`, OOC physical route | Vivado CLB LUT | `52806` |
| PPA4 compact route | `boom_l3_ppa4_system_top` | Vivado CLB LUT | `49452` |
| PPA4 compact route | `core_u` only | Hierarchical total LUT | `48458` |
| PPA4 compact route | `memory_u` adapter only | Hierarchical total LUT | `931` |

## Explanation

The `211204` number is an HLS estimate, not a placed-and-routed Vivado fabric count. It is known to be pessimistic for this generated design and does not account the same way as Vivado `CLB LUTs` after synthesis, logic optimization, mapping, placement, and routing.

The compact PPA4 route is close to the PPA3 OOC routed core LUT count. This is the strongest cross-check that the route did not collapse to an adapter-only design. Netlist hierarchy confirms `core_u` remains the dominant resource consumer at `48458` LUT.

## What Was Optimized

| Optimization Class | Evidence | Interpretation |
| --- | --- | --- |
| HLS estimate vs Vivado resource accounting | `211204` HLS LUT vs `52806` PPA3 OOC CLB LUT | Different tools and estimation stages; not a direct apples-to-apples comparison. |
| Normal Vivado flatten/rebuild optimization | `synth_design -flatten_hierarchy rebuilt` | Some module boundaries and names are transformed; hierarchy audit must use both hierarchy and cell matching. |
| Adapter compaction | Initial adapter synth `720621` LUT failed; compact adapter route `931` adapter LUT | PPA4 removed synthetic byte-array memory blow-up. |
| Product logic preservation | `core_u=48458` LUT and `29567` FF; black boxes `0` | Core hardware is present in compact route. |
| Full workload proof | B3I program RTL `6/6 PASS`; multi-outstanding program workload pending | Functional signoff cannot be inferred from resources alone. |

## Classification

`L3_PPA5_PPA4_IMPLEMENTATION_CLASS=FULL_CORE_WITH_COMPACT_ONCHIP_ADAPTER_PHYSICAL_EVIDENCE`

`L3_PPA5_FULL_CORE_PRESENT=PASS_NETLIST_PRESENT`

`L3_PPA5_CRITICAL_LOGIC_PRESERVED=PASS_NETLIST_HIERARCHY_PRESENT_FUNCTIONAL_MULTI_OUTSTANDING_PROGRAM_PENDING`

The compact implementation is not classified as `ADAPTER_OR_PARTIAL_CORE`, because routed hierarchy shows the full generated core is present and dominates resource usage. It is also not a full signoff result, because PPA4 IMEM returns NOPs and PPA5 multi-outstanding end-to-end workload remains incomplete.
