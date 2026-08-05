# Regression Before M3A

The accepted M2C evidence is frozen rather than rerun:

- Standalone multiply directed: 51/51 PASS.
- Standalone multiply random: 131072 vectors, zero mismatch.
- Integrated execute directed: 30/30 PASS.
- Persistent execute random: 131072 vectors, zero mismatch.
- Canonical csynth: 8/8 PASS.
- Focused generated RTL: 10/10 PASS.
- Full-core generated RTL: 2/2 PASS.
- W4 topology: 3 completion sources, 2 PRF writes, 3 wakeups, 3 bypasses.

M3A intentionally does not rerun full-core RTL or full-core csynth.
