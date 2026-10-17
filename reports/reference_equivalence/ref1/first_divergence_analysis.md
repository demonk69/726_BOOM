# REF1 First Divergence Analysis

Normalization keeps all commits at PC >= `0x80000000` and retains raw traces under `normalized_commit_traces/raw/`. Reset/ROM instruction differences are outside the common program interval.

## independent_alu

No architectural commit divergence inside common program interval; commits reference=9 HLS=9; commit_count_match=True; tohost_memory_match=True.

Reference terminal cycle=1171; HLS terminal cycle=32439; last-common-commit latency delta (HLS-reference)=31248 cycles. These are reported values, not a cycle-equality criterion.

## raw_chain

No architectural commit divergence inside common program interval; commits reference=9 HLS=9; commit_count_match=True; tohost_memory_match=True.

Reference terminal cycle=1171; HLS terminal cycle=32438; last-common-commit latency delta (HLS-reference)=31247 cycles. These are reported values, not a cycle-equality criterion.

## branch_taken

No architectural commit divergence inside common program interval; commits reference=10 HLS=10; commit_count_match=True; tohost_memory_match=True.

Reference terminal cycle=1182; HLS terminal cycle=38059; last-common-commit latency delta (HLS-reference)=36857 cycles. These are reported values, not a cycle-equality criterion.

## branch_not_taken

No architectural commit divergence inside common program interval; commits reference=11 HLS=11; commit_count_match=True; tohost_memory_match=True.

Reference terminal cycle=1173; HLS terminal cycle=37130; last-common-commit latency delta (HLS-reference)=35937 cycles. These are reported values, not a cycle-equality criterion.

## nested_branch

No architectural commit divergence inside common program interval; commits reference=11 HLS=11; commit_count_match=True; tohost_memory_match=True.

Reference terminal cycle=1195; HLS terminal cycle=43712; last-common-commit latency delta (HLS-reference)=42497 cycles. These are reported values, not a cycle-equality criterion.

A failure row identifies the first actual common-program mismatch; it is not normalized away.
