# baseline.tcl - Conservative HLS directives
# Priority: ensure synthesis succeeds without aggressive optimization

open_project boom_hls_baseline

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

open_solution "solution_baseline"

set_part [expr {$::env(FPGA_PART)}]
create_clock -period [expr {$::env(CLOCK_PERIOD)}] -name default

# Conservative: no unroll on main loop
# CORE_CYCLE is inside boom_core_top with while(true)

# Keep module hierarchy
config_compile -pipeline_loops 0

# Map large arrays to BRAM
set_directive_bind_storage -type bram boom_core_top state
set_directive_bind_storage -type bram boom_core_top state.int_rf
set_directive_bind_storage -type bram boom_core_top state.fp_rf
set_directive_bind_storage -type bram boom_core_top state.rob.entries
set_directive_bind_storage -type bram boom_core_top state.issue.mem_iq.entries
set_directive_bind_storage -type bram boom_core_top state.issue.alu_iq.entries
set_directive_bind_storage -type bram boom_core_top state.issue.fpu_iq.entries
set_directive_bind_storage -type bram boom_core_top state.rename.int_map_table.map_table
set_directive_bind_storage -type bram boom_core_top state.rename.int_map_table.br_snapshots

csynth_design

close_project
