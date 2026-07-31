# Gate 4.0 W3 Critical Path Analysis

Vitis HLS 2021.2 estimates for `xczu7ev-ffvc1156-2-e` at a 10 ns target. Paths are the longest state paths parsed from copied `*.verbose.sched.rpt` reports.

## synth_issue_top

- Estimated clock period: **4.570 ns**
- Longest verbose state path: **4.57 ns**
- Evidence: `synth_issue_top/verbose/issue_module.verbose.sched.rpt`, state `12`

```text
'load' operation ('uop_iq_type_load', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:558) on array 'state_issue_alu_iq_entries_uop_iq_type' [292]  (0.677 ns)
'icmp' operation ('icmp_ln558_4', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:558) [317]  (0.849 ns)
'and' operation ('and_ln558_3', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:558) [320]  (0.287 ns)
'and' operation ('and_ln558_7', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:558) [328]  (0.287 ns)
'select' operation ('select_ln558', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:558) [329]  (0 ns)
'select' operation ('select_ln558_1', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:558) [331]  (0.287 ns)
'select' operation ('port', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:565) [346]  (0.287 ns)
'icmp' operation ('icmp_ln632', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) [347]  (0.446 ns)
'and' operation ('and_ln632_1', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) [365]  (0 ns)
'and' operation ('and_ln632_2', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) [366]  (0 ns)
'and' operation ('and_ln632_3', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) [367]  (0.287 ns)
'or' operation ('or_ln632_2', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) [368]  (0.287 ns)
'select' operation ('int_index', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) [370]  (0.449 ns)
'store' operation ('store_ln632', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632) of variable 'int_index', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:632 on local variable 'int_index' [374]  (0.427 ns)
```

## synth_execute_top

- Estimated clock period: **5.081 ns**
- Longest verbose state path: **5.08 ns**
- Evidence: `synth_execute_top/verbose/execute_module.verbose.sched.rpt`, state `2`

```text
'load' operation ('state_int_rf_load', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:748) on array 'state_int_rf' [22]  (1.24 ns)
'select' operation ('rs1', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:748) [23]  (0.424 ns)
'mul' operation ('mul_ln776', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:776) [49]  (3.42 ns)
```

## synth_rob_top

- Estimated clock period: **1.829 ns**
- Longest verbose state path: **1.83 ns**
- Evidence: `synth_rob_top/verbose/rob_allocate.verbose.sched.rpt`, state `1`

```text
'load' operation ('uop.queue.rob_idx', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:533) on static variable 'state_rob_tail' [20]  (0 ns)
'add' operation ('add_ln548', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:548) [37]  (0.789 ns)
'icmp' operation ('icmp_ln549', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:549) [39]  (0.753 ns)
blocking operation 0.287 ns on control path)
```

## synth_lsu_top

- Estimated clock period: **3.474 ns**
- Longest verbose state path: **3.47 ns**
- Evidence: `synth_lsu_top/verbose/synth_lsu_top.verbose.sched.rpt`, state `1`

```text
fifo read operation ('dmem_resp_in_read', /home/lab_726/Xilinx/Vitis_HLS/2021.2/common/technology/autopilot/hls_stream_39.h:144) on port 'dmem_resp_in' (/home/lab_726/Xilinx/Vitis_HLS/2021.2/common/technology/autopilot/hls_stream_39.h:144) [22]  (1.64 ns)
fifo write operation ('write_ln173', /home/lab_726/Xilinx/Vitis_HLS/2021.2/common/technology/autopilot/hls_stream_39.h:173) on port 'pipe_3' (/home/lab_726/Xilinx/Vitis_HLS/2021.2/common/technology/autopilot/hls_stream_39.h:173) [23]  (1.84 ns)
```

## boom_core_top

- Estimated clock period: **5.898 ns**
- Longest verbose state path: **5.9 ns**
- Evidence: `boom_core_top/verbose/lsu_module.verbose.sched.rpt`, state `7`

```text
'load' operation ('address', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1281) on array 'state_rob_entries_memory_address' [88]  (1.24 ns)
'lshr' operation ('lshr_ln1112', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1112) [103]  (1.54 ns)
'and' operation ('value', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1112) [107]  (0.379 ns)
'xor' operation ('xor_ln1105', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1105) [111]  (0 ns)
'sub' operation ('sub_ln1105', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1105) [112]  (1.08 ns)
'select' operation ('select_ln1281', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1281) [115]  (0 ns)
'select' operation ('value', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1111) [116]  (0.424 ns)
'store' operation ('store_ln1286', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1286) of variable 'value', /home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1111 on array 'state_rob_entries_memory_data' [130]  (1.24 ns)
```
