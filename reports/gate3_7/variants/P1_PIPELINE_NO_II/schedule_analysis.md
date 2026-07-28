# P1_PIPELINE_NO_II Schedule Analysis

- Requested II: `auto`
- Achieved II: `NOT_REPORTED`
- Target II in report: `NOT_REPORTED`
- CORE_CYCLE pipelined: `NOT_REPORTED`
- Last completed pass: `Checking Synthesizability`
- Stage classification: `PRESYN2_IF_CONVERSION`
- Last observable operation: `INFO: [XFORM 203-401] Performing if-conversion on hyperblock to (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:973:1) in function 'boom::older_store_in_rob'... converting 100 basic blocks.`
- Warning count: 14
- Automatic partition count: 0
- Inlining records: 104
- Memory-promotion records: 0
- Loops marked implied complete-unroll: 65
- Completed loop-unroll records: 57
- Functions marked unroll-all for pipelining: 7
- Incomplete variable-bound unrolls: 8
- Maximum reported HLS current allocation: 1354.752 MB
- Memory-port conflict records: 0
- Dependency-message records: 0
- II-violation records: 0

## Relevant Messages

- `WARNING: [HLS 200-40] No /home/lab_726/boom/hls_boom/build/gate3_7/hls_projects/P1_PIPELINE_NO_II/solution_pipeline/solution_pipeline.aps file found.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_1132_1' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:1134) in function 'boom::rob_commit_module' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_802_3' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:802) in function 'boom::kill_lsu_state' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_803_4' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:803) in function 'boom::kill_lsu_state' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_567_5' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:565) in function 'boom::issue_module' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_594_2' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:593) in function 'boom::execute_module' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_742_2' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:742) in function 'boom::compact_issue_queue' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_523_2' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:523) in function 'boom::compact_iq' completely: variable loop bound.`
- `WARNING: [HLS 200-932] Cannot unroll loop 'VITIS_LOOP_685_1' (/home/lab_726/boom/hls_boom/src/boom_core_merged.cpp:687) in function 'boom::branch_free_preg_unique' completely: variable loop bound.`
- `WARNING: [HLS 200-805] An internal stream 'pipe.0' with default size can result in deadlock. Please consider resizing the stream using the directive 'set_directive_stream' or the 'HLS stream' pragma.`
- `WARNING: [HLS 200-805] An internal stream 'pipe.1' with default size can result in deadlock. Please consider resizing the stream using the directive 'set_directive_stream' or the 'HLS stream' pragma.`
- `WARNING: [HLS 200-805] An internal stream 'pipe.2' with default size can result in deadlock. Please consider resizing the stream using the directive 'set_directive_stream' or the 'HLS stream' pragma.`
- `WARNING: [HLS 200-805] An internal stream 'pipe.3' with default size can result in deadlock. Please consider resizing the stream using the directive 'set_directive_stream' or the 'HLS stream' pragma.`
- `WARNING: [HLS 200-805] An internal stream 'pipe.4' with default size can result in deadlock. Please consider resizing the stream using the directive 'set_directive_stream' or the 'HLS stream' pragma.`
