# Full LSU Gate Plan

- L1: parameterized canonical LDQ/STQ, 4/4, 8/8, 16/16 synthesis, stable ownership/generation, allocation/free/squash/reset/exception lifecycle only.
- L2: conservative forwarding. Search only older stores; youngest matching older store wins. Forward only when all requested bytes are covered by one store. Otherwise block; do not merge memory and forwarded bytes initially.
- L3: multiple outstanding loads with request tag `{lq_index,lq_generation,transaction_id}` and exact response routing. A killed response is consumed and dropped.
- L4: allow a load past known non-alias older stores; initially block unknown addresses and known aliases. A later store-address resolution checks younger executed loads and raises an internal memory-order recovery event.
- L5: full preservation, stress, generated RTL, timing, and cumulative PPA acceptance.

Recommended replay ownership is two-level: local LSU retry for transient backpressure/non-ordering retry; ROB-owned squash and frontend refetch for a detected ordering violation. The latter is a non-architectural recovery cause and must not be encoded as an exception.
