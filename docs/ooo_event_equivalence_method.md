# Out-Of-Order Event Equivalence Method

Gate 3.1A replaces the old global event-order comparison with a dynamic-uop partial-order comparison.

An out-of-order BOOM trace may legally show a branch resolve, execute, or writeback for a younger instruction before older instructions commit. That is not a functional error by itself. Commit remains the architectural serialization point and must stay in program order.

## Dynamic Uop Identity

Dynamic uops are matched across BOOM and HLS by committed program-order identity for the current finite loadmem traces, then refined with available event metadata:

- commit ordinal
- PC
- instruction
- occurrence index
- BOOM ROB index and estimated ROB wrap generation when the trace exposes them
- BOOM allocation cycle when it can be associated without guessing
- branch mask and branch tag when present

PC, instruction, or ROB index alone are never sufficient. If an event cannot be associated without relying on a guess, the event is marked `INSUFFICIENT_IDENTITY`.

## Checked Constraints

The corrected matcher checks only real causality and architectural constraints:

- Per-uop order: allocation or branch resolve must not occur after that same uop commits.
- Branch order: branch resolve may occur before older commits.
- Commit order: committed uops must be in program order with no duplicates or skipped committed prefix entries.
- RAW/WAR/WAW: checked only where producer, consumer, physical-register, issue, and wakeup signals exist; otherwise the result is `INSUFFICIENT_SIGNAL`.
- Branch squash: mispredicted-path committed architectural state is checked through the committed PC/instruction stream; raw wrong-path allocation identity remains limited because BOOM `rob_allocate` records do not include PC/instruction.

## Legacy Global-Order Result

The old `boom_vs_hls_csim_event_diff.csv` result is preserved. Its BOOM-vs-HLS failures are classified by Gate 3.1A as `VALIDATION_METHOD_FALSE_POSITIVE` for the current traces: they compare unrelated dynamic events at the same global stream position instead of matching the same uop and checking valid partial-order constraints.

This does not prove full microarchitectural equivalence. The current traces still lack decode, rename, IQ insert/select, full writeback/wakeup, complete, and enough wrong-path identity signals to close all RAW/WAR/WAW and squash constraints.
