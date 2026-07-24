# area.tcl - Area-optimized HLS directives
# Reduce resource usage while maintaining functionality

open_project boom_hls_area

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

open_solution "solution_area"

set_part [expr {$::env(FPGA_PART)}]
create_clock -period [expr {$::env(CLOCK_PERIOD)}] -name default

# Reduce unrolling
config_unroll -tripcount_threshold 0

# Map ROB to URAM if available, otherwise BRAM
set_directive_bind_storage -type bram boom_core_top state.rob.entries

# Map issue queues to BRAM
set_directive_bind_storage -type bram boom_core_top state.issue.mem_iq.entries
set_directive_bind_storage -type bram boom_core_top state.issue.alu_iq.entries
set_directive_bind_storage -type bram boom_core_top state.issue.fpu_iq.entries

# Limit multiplier/divider instances
set_directive_allocation -limit 1 -type operation boom_core_top mul
set_directive_allocation -limit 1 -type operation boom_core_top udiv

# Map register files to BRAM
set_directive_bind_storage -type bram boom_core_top state.int_rf
set_directive_bind_storage -type bram boom_core_top state.fp_rf

# Preserve module boundaries
config_rtl -reset state -reset_async

csynth_design

close_project
