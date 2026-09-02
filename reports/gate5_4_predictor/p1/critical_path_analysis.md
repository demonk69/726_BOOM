# Predecode Critical Path Analysis

Vitis HLS 2021.2 reports 2.442 ns for scalar predecode and 4.304 ns for the two-lane packet helper at a 10 ns target. Both have zero-cycle combinational latency, interval 1, `PipelineType=no`, and infer no FF, memory, BRAM, DSP, or FIFO.

The scalar path is opcode/funct3 classification plus immediate bit assembly, sign extension, and one 64-bit PC add. The packet path instantiates two predecode modules (616 LUT each in that context) and adds 276 LUT of selection/mask expressions, for 1508 LUT total. Thus the current future estimate is two parallel decode copies plus earliest-lane mux/mask logic; no sharing optimization is attempted in P1.

These are standalone HLS estimates, not a claim that full-core PPA changed. The preservation Frontend top remains independently synthesizable and has no predecode instance.
