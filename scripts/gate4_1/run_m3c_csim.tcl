set root $::env(HLS_PROJECT_ROOT)
open_project -reset m3c_full_core_csim
set_top boom_core_top
add_files -cflags "-std=c++11 -I$root/include" "$root/src/boom_core_merged.cpp"
add_files -tb -cflags "-std=c++11 -I$root/include" "$root/tb/differential/rv64m_full_core_tests.cpp"
open_solution -reset solution_m3c_csim
set_part $::env(FPGA_PART)
create_clock -period $::env(CLOCK_PERIOD) -name default
csim_design
close_project
exit
