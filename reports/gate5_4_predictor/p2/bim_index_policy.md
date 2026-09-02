# BIM Index Policy

For every supported power-of-two depth:

```text
index = (pc >> 1) & (entries - 1)
```

Bit 0 is dropped because both 16-bit and 32-bit instructions are halfword aligned. Index widths are 6, 7, 8, and 9 for 64, 128, 256, and 512 entries. The exact selected CFI PC is used, not packet base PC. Aliasing is intentional BIM behavior; tags are not part of this foundation.

The response metadata token is the derived index. Update acceptance re-derives the index from update PC and requires token equality, preventing malformed or stale metadata from training another entry.
