# Product Integration PPA Risk

```text
PRODUCT_INTEGRATION_PPA_RISK=MEDIUM
```

Standalone figures are not additive full-core deltas: P1 scalar is 639 LUT/2.442 ns, two-lane helper 1508 LUT/4.304 ns, P2 is 684 LUT/465 FF/2.989 ns, and F1 is 3006 LUT/1358 FF/3.788 ns. Full core is 135953 LUT/33373 FF/16 BRAM/3 DSP/6.341 ns.

| Integration area | Risk | Reason/control |
|---|---|---|
| parallel two-lane predecode | medium | 4.304 ns standalone helper; isolate before predictor state |
| packet arbitration/masking | medium | affects next-PC and FB capacity logic; freeze before admission |
| predictor pending control | low-medium | small state but redirect/reset fan-in |
| FTQ references through pipeline | medium | distributed FF and mux/fanout growth |
| ROB actual metadata | low | direction/resolved bits only for foundation |
| redirect comparator/mux | high timing sensitivity | recovery priority and target mux already global |
| FTQ storage | medium | LUTRAM selected; integrated ports/order may alter inference |
| BIM state | low-medium | LUTRAM and same-index forwarding |

The aggregate is MEDIUM because resource percentages are modest relative to the core, but timing and semantic fanout span Frontend, ROB, recovery, and Commit. Do not combine IMEM response, two-lane predecode, BIM lookup, mask, and enqueue in one cycle. No CORE_CYCLE pipeline, DATAFLOW, false dependence, or complete array partition is authorized.
