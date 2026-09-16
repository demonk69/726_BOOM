# Independent Reset Depths

`RESET_LSU` uses separate `lq_index` and `sq_index` controller fields. Each iterator is independently guarded and terminates at its own depth. Equal 8/8 reset latency remains eight queue cleanup steps; asymmetric configurations run until both independent iterators finish.

Native/CSim 4/16 and 16/4 reset checks passed. Generated full-core RTL for both asymmetric configurations passed power-on reset and load/store smoke.
