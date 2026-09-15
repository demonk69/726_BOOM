# G6.0 L1 Entry Contract

Scope is exactly `PARAMETERIZED_CANONICAL_LQ_SQ_AND_STABLE_IDENTITY`.

L1 may implement parameterized canonical LDQ/STQ state; derived indices; per-entry 16-bit generations; owner tuple `{queue index,generation,ROB index,ROB allocation ID}`; allocation/free; correct-branch mask clearing; younger mispredict squash; exception/global-flush cleanup; and reset cleanup. Default functional configuration remains 8/8; synthesis acceptance is 4/4, 8/8, and 16/16.

L1 must not implement forwarding, multiple outstanding loads, ordering speculation, violation detection, replay, DCache, MMU, pipeline stages, or scheduling directives. Existing external memory behavior remains single-outstanding. Stable queue generations must be checked before any delayed event can mutate an entry; ROB allocation identity remains mandatory rather than replaced by queue generation.

The current staged reset indexes LDQ and STQ together and terminates at `LDQ_DEPTH` (`src/reset.cpp:242-264`), which is safe only because both are 8. L1 must use independently bounded LDQ and STQ cleanup so separately parameterized depths cannot cause an out-of-bounds access or leave entries uncleared. Queue count widths must represent `0..DEPTH`, while entry index widths represent `0..DEPTH-1`.
