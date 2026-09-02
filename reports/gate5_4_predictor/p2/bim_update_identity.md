# BIM Update Identity

An update trains only when all conditions hold:

1. `update.valid` is true.
2. `update.commit_qualified` is true.
3. `update.cfi_type == CFI_CONDITIONAL_BRANCH`.
4. `update.generation == active_generation`.
5. `update.metadata_token == ((update.pc >> 1) & (entries - 1))`.

Squashed/speculative, stale-generation, nonconditional, and metadata/PC-mismatched updates are ignored. The request token identifies a blocking request/response; the metadata token identifies a BIM entry. They are not interchangeable.

When a qualified update collides with a lookup of the same index in one logical step, the lookup receives the newly saturated counter value (`UPDATE_FORWARD_NEW_VALUE`).
