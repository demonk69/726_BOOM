# PPA6 Multi-Outstanding Program

The PPA6 program is a normal RV64 program loaded through the IMEM model. It starts from reset through the core's reset trampoline, enters `0x80000000`, issues four independent loads from `0x80000100..0x80000118`, computes a sum, performs an older store followed by a younger same-address load, executes compressed instructions and a compressed jump redirect, writes final memory signatures, and commits a `tohost=1` store.

Expected data values: `3`, `5`, `7`, `11`; first sum `26`; store-forward value `90`; final `a0=119`.

The DMEM model accepts real AXIS handshakes, records transaction IDs, intentionally stalls requests for legal backpressure, stalls the committed same-address store to attempt the forwarding-only path, and returns at least one response out of request order when multiple loads are outstanding.

This testbench observes internal outstanding count for audit only; it does not write ROB, LQ, outstanding, completion, or register-file state.

Runner: `ppa6_run_full_core_e2e.sh` (`f71960be293953557779d5520a5586a561bf606eac659126e5ee41772e0ac01d`)

Testbench: `ppa6_multi_outstanding_tb.sv` (`6c363b52b01a6b1d99a86e11708d6a698abf80d8d46ab18e6f75b302bee04d4b`)
