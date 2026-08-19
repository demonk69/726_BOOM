# Packet Constructor PPA Risk

## Accepted Reference

| Block | LUT | FF | BRAM | DSP | Estimated period ns |
|---|---:|---:|---:|---:|---:|
| RVC decompressor | 1022 | 0 | 0 | 0 | 1.845 |
| Depth-8 Fetch Buffer | 1179 | 846 | 0 | 0 | 4.574 |
| B2 Frontend | 2761 | 790 | 0 | 0 | 5.168 |
| B2 full core | 129885 | 29194 | 16 | 3 | 6.341 |

These are Vitis HLS estimates, not additive post-route costs. The Frontend report-level gap to the current full-core period is 1.173 ns.

## Width 2

To produce C+C in one call, two parcel classification/decompression paths must be available concurrently. Time-sharing one decompressor over two calls preserves the current scalar bottleneck and defeats the packet objective. HLS may share common logic, but the architecture must budget approximately a second RVC combinational path plus:

- lower/upper parcel PC generation;
- two complete metadata paths and contiguous valid mask;
- carry-completion versus ordinary-lower selection;
- first-fault termination and lane-1 suppression;
- a stable two-entry packet holding register under atomic enqueue backpressure.

The standalone buffer already contains four-lane compaction/write logic, so width 2 does not require widening its API. Risk is low-to-moderate in area and timing. The main timing concern is a chain of carry select, decompression, fault termination, metadata mux, and packet admission. B3I must measure `synth_frontend_top`, focused constructor top, and canonical full core against the existing 6.341 ns result; this review does not claim timing closure.

## Width 4

Single-response width 4 cannot activate more than two lanes, so any extra selection/hold logic has no throughput return. A fully utilized width 4 requires multi-response aggregation, adding packet occupancy/age, multiple response identities or committed ordering state, partial publication policy, larger fault selection, and a wider flush cone. Atomic four-entry admission can also wait for up to four free slots while dequeue remains one-wide.

Four decompressor instances are not justified by a two-parcel response. Two are the maximum useful concurrent instances. Additional time-multiplexing or aggregation increases state and transaction latency rather than response bandwidth.

## Risk Decision

- Width 1: no PPA risk, but retains measured mixed/cross-word parser bubbles.
- Width 2: bounded, directly useful risk; no response RAM and no protocol widening.
- Width 4 unpack-only: moderate logic risk with zero additional reachable lanes.
- Width 4 aggregated: high area/control/timing risk and overlaps the existing Fetch Buffer; defer until an actual wider ICache/fetch response exists.

No optimization directive, DATAFLOW region, false dependence, complete array partition, or `CORE_CYCLE` pipeline is recommended.
