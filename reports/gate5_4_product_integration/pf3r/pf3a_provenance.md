# PF3A Current-Source Provenance

- Modular source/header aggregate hash used by current canonical/B3I RTL:
  `be4820e746b99f658fc2f52f684b858bdf30051e6a283140e91ad5eda0fb07b8`.
- `src/frontend.cpp`: `935a7426d4e9fa28cb6a965a7ad1a64052c7b6bcf7e48a04c42c0b92482541ba`.
- `src/boom_all.cpp` excluded and unchanged:
  `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- Canonical csynth report:
  `f9ac067d6ec035b81acbe068c742d6df4d9b90e3032697fc1d672e7203fcd3c4`.
- Canonical generated `boom_core_top.v`:
  `e1c6aa922e615936bc36cf9fefa3ac9b39030b896fc1156d12156fb82345cab5`.
- Canonical compile define: `-DBOOM_FTQ_STORAGE_LUTRAM`.
- Canonical artifact root:
  `/tmp/boom_hls/pf3a/boom_hls_gate5_4_pf3a_canonical_boom_core_top/solution_module`.

All PF3A full-core, PF2 full-core, PF1 full-core, and Gate 5.3 B3I RTL runs
referenced that canonical generated RTL directory.

Current-source focused RTL logs were retained at:

- PF3: `/tmp/boom_hls/pf3a/preserved/gate5_4_product_integration/pf3/focused_rtl.bXd1kc/xsim.log`.
- PF2: `/tmp/boom_hls/pf3a/preserved/gate5_4/pf2/focused_rtl.lkJwzu/xsim.log`.
- PF1: `reports/gate5_4_product_integration/pf1/logs/pf1_exception_rtl.log`.
