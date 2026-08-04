# W4 Wakeup And Bypass Semantics

The W4 integer completion network has three fixed publication entries for wakeup and three for bypass. Each carries valid, physical destination, value, ROB index, allocation identity, branch mask, and source. The three possible sources are the named retained load response, MEM execute result, and INT execute result.

Publication occurs after allocation-owner validation, branch resolution/kill, oldest precise-exception fencing, and same-destination validation. Equal destination/equal value coalesces; unequal values fault and publish nothing. Operand resolution priority is x0, current bypass, then nonbusy PRF. Random evidence observed peaks of three wakeups and three bypasses, with 17,309 exact publications of each and no stale side effects.

This is not BOOM fast-wakeup/replay cycle equivalence. It is the verified HLS subset behavior; the FP lane remains inactive.
