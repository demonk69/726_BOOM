# Gate 3.10 Critical Path Analysis

The Gate 3.9 live Vitis HLS 2021.2 schedule database was parsed directly. This is HLS scheduling evidence, not Vivado post-synthesis or post-route STA; no physical top-20 endpoint report exists.

The longest state-local cone is `lsu_module` state 5 at 5.90 ns: ROB address RAM read, variable shift, mask, sign extension, selection, and ROB data RAM write. The second is `execute_module` state 3 at 5.87 ns: PRF read, operand selection, 32x32 multiply, and result write.

The generated hierarchy and helper reports confirm that LSU load response extraction sets the 5.898 ns top estimate. Reset logic is 1.875 ns and is not a normal-path timing limiter.

`critical_path_inventory.csv` contains the derived top 20 state-local schedule paths. These rows must not be described as routed FPGA timing paths.
