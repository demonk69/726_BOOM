# PF3 Exception FTQ Lifetime

PF1 captures `mepc`, `mcause`, `mtval`, and redirect state before scheduling an
FTQ redirect for the fault owner. The redirect retains the owner lane and kills
younger entries. Only after canonical F1 accepts that redirect is the deferred
owner retire event issued on the next Frontend step. This prevents early owner
release while permitting ordered reclaim after trap ownership is established.
