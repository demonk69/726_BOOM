# run_csim.tcl - C simulation script
open_project boom_hls_csim
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

open_solution "solution_csim"
set_part [expr {$::env(FPGA_PART)}]
create_clock -period [expr {$::env(CLOCK_PERIOD)}] -name default

csim_design

close_project
exit
