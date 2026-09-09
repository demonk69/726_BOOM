# PF3 Stale Reference Policy

Every reference carries a 32-bit F1 allocation generation in addition to the
5-bit slot and lane. Commit, squash, redirect, exception, and lookup compare the
generation before changing or exposing an entry. Slot wrap cannot alias an old
reference. Runtime reset invalidates all entry controls and advances the next
generation while preserving payload, so every pre-reset reference is stale.
