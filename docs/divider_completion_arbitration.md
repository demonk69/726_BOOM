# Divider Completion Arbitration

The Divider is a persistent INT subunit, not a fourth completion producer. Its terminal response is converted to the existing `ExecuteState::alu_results[INT_ISSUE_LANE]` payload and then follows `completion.int_execute`. W4 continues to perform ownership validation, oldest-age service, PRF writeback, wakeup, bypass, and ROB completion.

Ordinary ALU, MUL, and branch uops continue to use the INT lane while Divider arithmetic is busy. A normal one-cycle result already accepted into the INT execute slot wins that slot. The Divider response remains stable in `result_pending`, is not consumed, and retries after the slot drains. Once a response is pending at issue time, INT issue is backpressured for one publication opportunity so a new one-cycle producer cannot take its required slot. MEM issue and load response remain independent.

This is deterministic retention arbitration:

1. Existing INT execute-slot owner wins.
2. A held Divider response takes the next empty INT execute slot.
3. Normal INT issue resumes after Divider publication.
4. W4 applies oldest ROB age across the three retained completion sources; source index is its existing tie-break.

No ordinary INT result or Divider response needs an additional holding slot. The Divider response is consumed only when copied into the existing INT execute slot with a still-live matching allocation owner. Completion source count remains 3, PRF write ports remain 2, and wakeup, bypass, and ROB complete source counts remain 3.
