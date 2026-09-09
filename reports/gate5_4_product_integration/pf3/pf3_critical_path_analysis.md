# PF3 Critical Path Analysis

Canonical `boom_core_top` estimates 6.739 ns and fails the PF3 6.5 ns limit.
The full-core child `frontend_module` is also 6.739 ns, while
`execute_module` remains 6.341 ns. The standalone Frontend is 7.621 ns and the
focused PF3 wrapper is diagnostic only. This localizes the regression to the
integrated Frontend/FTQ admission cone rather than execute.

The current implementation calls canonical F1 before stamping and enqueueing
the packet, producing the conceptual response/allocation/reference/enqueue
path that PF3 explicitly required auditing. A future PF3 repair must introduce
an explicit registered admission/ready contract without changing atomicity or
enabling `CORE_CYCLE` pipelining. No timing repair is accepted in this run.
