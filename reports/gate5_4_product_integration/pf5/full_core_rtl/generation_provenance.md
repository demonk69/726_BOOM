# PF5 Current-Source Full-Core RTL Provenance

- Modular source/header SHA-256: `0e21d79d9d170e44397c3b7e1c5317acf417fe0188735098f5dba5e3c4a0be44`.
- `src/boom_all.cpp` excluded and untouched; generated merged source excluded from aggregate hash.
- Test-only full-core top enables Product FTQ and accepts fixture-only BIM seeds; product ports are unchanged.
- Product programs: `12/12 PASS`.
- Predicted-T younger-fault checks: actual-T masked and actual-NT precise refetch/take, `2/2 PASS`.
- Eligible committed conditional branches changed canonical BIM state; excluded CFI classes did not train.
