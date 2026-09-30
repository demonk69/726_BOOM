# PF6 Current-Source Full-Core RTL Provenance

- Modular source/header SHA-256: `7fa60c73d7c2a584ecb859e8d4f6ec5270ad5d8d6da9751030af6becdc68d808`.
- `src/boom_all.cpp` excluded and untouched; generated merged source excluded from aggregate hash.
- Test-only full-core top enables Product FTQ and accepts fixture-only BIM seeds; product ports are unchanged.
- RTL pass records retain the established `PF5_FULL_CORE_RTL_PASS` event name
  because PF6 reuses the PF5 predictor-training instrumentation and oracle.
- Product programs: `12/12 PASS`.
- Predicted-T younger-fault checks: actual-T masked and actual-NT precise refetch/take, `2/2 PASS`.
- Eligible committed conditional branches changed canonical BIM state; excluded CFI classes did not train.
