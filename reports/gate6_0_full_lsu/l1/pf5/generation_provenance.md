# PF5 Current-Source Full-Core RTL Provenance

- Modular source/header SHA-256: `b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618`.
- `src/boom_all.cpp` excluded and untouched; generated merged source excluded from aggregate hash.
- Test-only full-core top enables Product FTQ and accepts fixture-only BIM seeds; product ports are unchanged.
- Product programs: `12/12 PASS`.
- Predicted-T younger-fault checks: actual-T masked and actual-NT precise refetch/take, `2/2 PASS`.
- Eligible committed conditional branches changed canonical BIM state; excluded CFI classes did not train.
