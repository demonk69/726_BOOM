# Gate 5.3 B3I Baseline Manifest

- Branch: `gate3.8-rtl-verification`.
- Entry HEAD and accepted B2 commit: `c5e15cae358d6cbdfb1b4ce7e52f292b2538bbc3` (`Gate 5.3 B2: integrate fetch buffer with frontend`).
- B3 review commit: `UNCOMMITTED_WORKTREE_REVIEW`; review evidence was present under `reports/gate5_3_fetch_buffer/b3_review/` and SHA-256 bound in `source_hashes_before.txt` before B3I product edits.
- Accepted status: Gate 5.1, Gate 5.2 RVC, B1 Fetch Buffer, and B2 integration verified.
- Accepted topology: 32-bit IMEM response, one logically tracked request ownership, one-wide producer, depth-8 AUTO Fetch Buffer with CONTROL_ONLY reset, one-wide Decode/Dispatch/Commit.
- Accepted B2 full-core PPA: 129885 LUT, 29194 FF, 16 BRAM_18K, 3 DSP, 6.341 ns, `CORE_CYCLE Pipelined=no`.
- Entry index was empty. Historical dirty and untracked files were retained; `src/boom_all.cpp` was already modified and remains excluded.
- B3I candidate starts at `S0_NO_NEW_DIRECTIVE`.

Frozen prohibitions: no four-lane producer, multi-response aggregation, FTQ, predictor, BTB/BIM/TAGE/RAS, ICache, Decode/backend widening, Full LSU, FPU, DATAFLOW, false DEPENDENCE, complete ARRAY_PARTITION, or core-cycle pipeline.
