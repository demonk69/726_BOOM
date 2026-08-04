set root $::env(HLS_PROJECT_ROOT)
set part $::env(FPGA_PART)
set clk $::env(CLOCK_PERIOD)

open_project -reset boom_hls_prefix_trace_csim
set_top boom_core_top

add_files -cflags "-std=c++11 -I$root/include" "$root/src/boom_core_top.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/boom_core_step.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/reset.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/frontend.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/decode.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/rename.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/rob.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/issue.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/execute.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/branch.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/lsu.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/completion.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/commit.cpp"
add_files -cflags "-std=c++11 -I$root/include" "$root/src/csr.cpp"

add_files -tb -cflags "-std=c++11 -I$root/include" "$root/tb/differential/hls_prefix_trace_tb.cpp"

open_solution "solution_prefix_trace"
set_part $part
create_clock -period $clk -name default
csim_design

close_project
exit
