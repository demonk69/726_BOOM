# LSU CAM and PPA Risk

L1 cost scales linearly with queue state and index/control logic; no full address CAM is allowed. L2 forwarding introduces up to `LQ_DEPTH x SQ_DEPTH x 64 address-bit compare x byte-mask overlap x MEM issue width(1)` logical comparison exposure plus an age-qualified youngest-match priority mux. L3 adds `outstanding loads x tag compare` response routing. L4 adds store-address resolution against younger issued loads and violation selection, again approximately `LQ_DEPTH x SQ_DEPTH` comparisons.

Recommended physical banking:

- LQ: identity/control `{valid,ROB owner,generation,branch/age}`, address/size/sign searchable bank, and completion/data bank.
- SQ: identity/control/commit bank, searchable address bank, data bank, mask/size bank, and status bank.

This avoids whole-entry RMW and keeps wide data out of CAM cones. Use registers/distributed storage for 4/4 and 8/8. At 16/16, LUTRAM is reasonable for data/status payloads, but address/valid/age/mask metadata remains replicated or register-based for parallel search. Do not map the whole queue into one single-port BRAM.

Budgets below are cumulative from Gate6 baseline, never rolling:

| Gate | LUT review | LUT blocker | BRAM expectation | Period limit |
|---|---:|---:|---:|---:|
| L1 | 217705 (+2%) | 224108 (+5%) | 16 | 6.500 ns |
| L2 | 224108 (+5%) | 238340 (+11.7%) | 16 | 6.450 ns |
| L3 | 228375 (+7%) | 245451 (+15%) | 16 | 6.450 ns |
| L4 | 234780 (+10%) | 266795 (+25%) | 16; increase requires review | 6.500 ns |
| L5 | 234780 (+10%) | 266795 (+25%) | 16; final measured | 6.500 ns |
