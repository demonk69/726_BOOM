# L1 Lifecycle Validation

Validated allocation, full rejection, exact owner reclaim, empty state, more than 4x wrap, generation reuse, stale event rejection, branch squash, global flush, precise exception flush, reset, and count-valid equality. The 4/4 bounded model explored 137257 nodes. Default random campaign executed 256 seeds x 4096 cycles with all named counters zero.

The random campaign is a bounded queue reference model plus directed product-path tests; it is not a cycle-accurate full-core random memory model.
