# Gate 5.3 B3I Critical-Path Impact

- Status: **PASS**.
- `boom_core_top`: `6.341 ns`, unchanged from B2 `6.341 ns`; acceptance limit is `6.5 ns`.
- `CORE_CYCLE` remains unpipelined: top-level XML reports `PipelineType=no` and the `CORE_CYCLE` loop has no defined iteration latency or PipelineII.
- Resources: `135953 LUT / 33373 FF / 16 BRAM_18K / 3 DSP`; packet construction adds combinational/frontend state cost but does not move the accepted full-core estimated period.
- No end-to-end speedup is claimed because Decode/Dispatch/Commit remain one-wide.
