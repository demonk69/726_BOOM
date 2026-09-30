# Gate 6.0 L2 Results

## Verdict

`G6_0_L2_STORE_TO_LOAD_FORWARDING_VERIFIED`

The repaired 4/4, 8/8, and 16/16 configurations satisfy the frozen synthesis
gates. All required current-source functional and preservation checks pass.
The PF6 blocker was a stale testbench observation state, not a product failure.

`G6_0_L2_STORE_TO_LOAD_FORWARDING_VERIFIED=true`

## Product

- Policy: `YOUNGEST_OLDER_OVERLAPPING_SINGLE_STORE_FULL_COVER_ONLY`.
- Product/header aggregate SHA-256: `7fa60c73d7c2a584ecb859e8d4f6ec5270ad5d8d6da9751030af6becdc68d808`.
- Canonical merged source SHA-256: `25684f6a7ad91b40ea0f36389b1ea4d1abbbf16e0663174aec2daa02649a33fe`.
- Excluded working-tree `src/boom_all.cpp` SHA-256:
  `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
  This protected dirty file is not represented by the release commit blob.
- External outstanding-load policy remains one transaction.
- Partial forwarding, store merge, memory/forward merge, speculation,
  violation detection, and replay remain disabled.

## Functional Verification

- Native parameter tests: 4/4, 8/8, 16/16, 4/16, and 16/4 PASS.
- Native default random: 256 seeds x 8192 cases = 2,097,152 PASS.
- Vitis CSim parameter tests: all five configurations PASS.
- PATH_A focused RTL: 48/48 PASS against an independent oracle.
- PATH_B canonical full-core RTL: 6/6 PASS.
- PATH_B proves no DMEM request and no external pending transaction for a
  forwarded load, retained completion, exact value extension, ROB completion,
  LQ reclaim, conservative partial-overlap behavior, stale SQ/LQ owner
  rejection, and reset clearing.
- L1 frozen 80-case classification: 77 PRESERVED and 3
  SUPERSEDED_BY_L2_SEMANTICS (cases 26, 27, and 48).

## Preservation

- M3B directed: 167/167 PASS.
- M3C directed/random/program: PASS.
- W3: 15/15 suites, 400/400 PASS.
- W4 current-source multi-writeback: 13/13 PASS.
- RVC native, CSim, and fresh full-core RTL: 11/11 PASS each.
- PF6 fresh current-source full-core RTL: 12/12 programs PASS and mandatory
  predicted-taken fault checks 2/2 PASS. The harness observation state was
  updated from stale generated state 34 to the current equivalent state 35;
  product RTL and semantic checks were unchanged.

## Synthesis

| Configuration | LUT | FF | BRAM18K | DSP | Period | Status |
|---|---:|---:|---:|---:|---:|---|
| 4/4 | 208959 | 47345 | 16 | 3 | 6.071 ns | PASS |
| 8/8 | 207247 | 46664 | 16 | 3 | 6.071 ns | PASS |
| 16/16 | 211984 | 49382 | 16 | 3 | 6.071 ns | PASS |

Default 8/8 delta from T0R is +6176 LUT, +971 FF, 0 BRAM, 0 DSP, and
0.000 ns. LUT remains below the 224108 review threshold and 238340 blocker.
The repaired synthesis reports retain the 6.071 ns estimate in all three
configurations.

## Scope

Before the final commit-only release audit, no stage, commit, push, L3 work,
broad restore, or modification to `src/boom_all.cpp` was performed. The release
package subsequently uses an explicit 51-file manifest and continues to exclude
that protected working-tree file.

`NEXT_REQUIRED_ACTION=COMMIT_G6_0_L2`
