# Regression After M3A

- Divider directed: 104/104 PASS, zero mismatch.
- Divider arithmetic random: 256 seeds x 256 operations = 65536, zero mismatch.
- Per-operation random coverage: 8192 each for all eight operations.
- Divider protocol random: 128 seeds x 512 cycles = 65536, zero mismatch.
- Held-response checks: 1228; reset kills checked: 253.
- Standalone merged C++ compile: PASS.
- Canonical divider inclusion count: exactly one.
- `synth_divider_top` csynth: PASS.
- Full-core RTL: NOT RUN by M3A requirement.
- Full-core csynth: NOT RUN by M3A requirement.
- Protected execute/completion/ROB/LSU/issue/frontend hashes: unchanged.
- Existing task-before dirty `src/boom_all.cpp`: untouched and excluded.
