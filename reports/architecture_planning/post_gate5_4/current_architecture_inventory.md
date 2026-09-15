# Post-Gate5.4 Current Architecture Inventory

- Release: `65409c03ba84caf0e4b148247f913faf9af58c99`, branch `gate3.8-rtl-verification`.
- Accepted product: PF3 product FTQ, PF4 prediction/recovery, PF5 commit-qualified BIM training, PF6 full RTL/PPA closure.
- Backend preservation: RVC, RV64M, PF1 precise exceptions, W3 completion, W4 dual writeback, and minimal LSU tests.
- Core cycle is not pipelined. Canonical PPA is 213436 LUT, 47337 FF, 16 BRAM, 3 DSP, 6.341 ns.
- Current LSU is an 8-entry LDQ plus 8-entry STQ bookkeeping implementation. It permits one outstanding load, blocks a load behind every older valid `is_sta`, has coupled store address/data, and has no memory forwarding, ordering speculation, violation detection, or replay.
- External memory is an uncached ready/valid stream abstraction with 32-bit transaction IDs. DCache and MMU constants exist, but no product DCache/MMU behavior is part of this LSU.
- Historical dirty files, including `src/boom_all.cpp`, are not part of this review and were not changed.

`GATE5_4_PRODUCT_INTEGRATION_VERIFIED=true`
