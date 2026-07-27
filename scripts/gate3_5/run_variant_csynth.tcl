set root $::env(HLS_BOOM_ROOT)
set variant $::env(GATE35_VARIANT)
set top $::env(BOOM_HLS_TOP)
set part $::env(FPGA_PART)
set clk $::env(CLOCK_PERIOD)

open_project -reset boom_hls_gate3_5_${variant}_${top}
set_top $top
add_files -cflags "-std=c++11 -I$root/include" "$root/src/boom_core_merged.cpp"
open_solution -reset solution_module
set_part $part
create_clock -period $clk -name default
config_compile -pipeline_loops 0
csynth_design
close_project
exit
