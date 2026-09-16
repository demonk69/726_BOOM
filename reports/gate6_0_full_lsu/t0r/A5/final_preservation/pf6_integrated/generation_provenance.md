# PF5 Current-Source Full-Core RTL Provenance

- Modular source/header SHA-256: `492cc6257137595a910273e20f77c08693146ac927b342045c1a6e71ec6284a9`.
- `src/boom_all.cpp` excluded and untouched; generated merged source excluded from aggregate hash.
- Test-only full-core top enables Product FTQ and accepts fixture-only BIM seeds; product ports are unchanged.
- Product programs: `12/12 PASS`.
- Predicted-T younger-fault checks: actual-T masked and actual-NT precise refetch/take, `2/2 PASS`.
- Eligible committed conditional branches changed canonical BIM state; excluded CFI classes did not train.
