# performance.tcl - Performance-optimized HLS directives
# Use only after baseline synthesis succeeds
# Target: PIPELINE II=1 on CORE_CYCLE

open_project boom_hls_performance

set_top boom_core_top

add_files -cflags "-std=c++11 -I../include" ../src/boom_core_top.cpp
add_files -cflags "-std=c++11 -I../include" ../src/boom_core_step.cpp
add_files -cflags "-std=c++11 -I../include" ../src/frontend.cpp
add_files -cflags "-std=c++11 -I../include" ../src/decode.cpp
add_files -cflags "-std=c++11 -I../include" ../src/rename.cpp
add_files -cflags "-std=c++11 -I../include" ../src/rob.cpp
add_files -cflags "-std=c++11 -I../include" ../src/issue.cpp
add_files -cflags "-std=c++11 -I../include" ../src/execute.cpp
add_files -cflags "-std=c++11 -I../include" ../src/branch.cpp
add_files -cflags "-std=c++11 -I../include" ../src/lsu.cpp
add_files -cflags "-std=c++11 -I../include" ../src/commit.cpp
add_files -cflags "-std=c++11 -I../include" ../src/csr.cpp

add_files -tb -cflags "-std=c++11 -I../include" ../tb/tb_boom_core.cpp

open_solution "solution_performance"

set_part [expr {$::env(FPGA_PART)}]
create_clock -period [expr {$::env(CLOCK_PERIOD)}] -name default

# Target II=1 on main loop
set_directive_pipeline -II 1 boom_core_top/CORE_CYCLE

# Unroll decode/dispatch width-1 loops (width=1 so trivially unrolled)
# When width > 1, unroll all width-lane loops
# For DECODE_WIDTH=1, the loop is single-iteration

# Array partitioning for Map Table
set_directive_array_partition -type complete -dim 1 boom_core_top state.rename.int_map_table.map_table
set_directive_array_partition -type complete -dim 1 boom_core_top state.rename.int_free_list.busy_table

# Dual-port BRAM for register files
set_directive_bind_storage -type ram_2p boom_core_top state.int_rf
set_directive_bind_storage -type ram_2p boom_core_top state.fp_rf

# Partition issue queue busy tables
set_directive_array_partition -type complete -dim 1 boom_core_top state.issue.mem_iq.entries
set_directive_array_partition -type complete -dim 1 boom_core_top state.issue.alu_iq.entries
set_directive_array_partition -type complete -dim 1 boom_core_top state.issue.fpu_iq.entries

csynth_design

close_project
