# PF6 and RVC RTL Provenance

## Shared Inputs

- Product/header aggregate: `7fa60c73d7c2a584ecb859e8d4f6ec5270ad5d8d6da9751030af6becdc68d808`.
- Merged source: `25684f6a7ad91b40ea0f36389b1ea4d1abbbf16e0663174aec2daa02649a33fe`.
- LQ/SQ depth: 8/8.
- Vitis HLS: 2021.2 build 3367213.
- Part: `xczu7ev-ffvc1156-2-e`.
- Clock: 10 ns.
- Shared flags: C++11, project include path, FTQ LUTRAM, predictor LUTRAM.
- Generated Verilog files: 91 in each build.

## Differences

- PF6 top: `boom_core_pf4_rtl_top`; it adds the test-only predictor seed AXIS
  port and Product FTQ observations.
- RVC top: canonical `boom_core_top`.
- RVC explicitly passes `LQ_DEPTH=8` and `SQ_DEPTH=8`; PF6 obtains the same
  values from `boom_config.hpp` defaults.

## RTL Hashes

- PF6 generated Verilog aggregate: `9d04e7814039f281f8d5f2613af9095dbc0de2de7c4548ed7f3999963fbb33bd`.
- PF6 top Verilog: `549e9db4eb06a4b76757049a4d0fca19720eb91f15d23d68da4c47282a343366`.
- Repaired 8/8/RVC generated Verilog aggregate: `39d6a00375988c58f362dcb9da4b0465dcceb597d709a697ce31838e4a0c854a`.
- Repaired 8/8 canonical top Verilog: `bf676f3efa1e84e9bc91496429f0bfce607c533aeae9fe0e92a9a2f408f895e2`.

`PF6_RVC_PRODUCT_SOURCE_MATCH=true`

`PF6_RVC_BUILD_CONFIG_MATCH=false`

`PF6_RVC_RTL_HASH_MATCH=false`

No PF6 HLS rerun was performed during repair. The existing current-source PF6
RTL and snapshot were reused; only the SystemVerilog harness was recompiled
and elaborated.
