set root $::env(HLS_BOOM_ROOT)
set variant $::env(GATE36_VARIANT)
set top $::env(BOOM_HLS_TOP)

open_project -reset boom_hls_gate3_6_${variant}_${top}
set_top $top
add_files -cflags "-std=c++11 -I$root/include" "$root/src/boom_core_merged.cpp"
open_solution -reset solution_variant
set_part $::env(FPGA_PART)
create_clock -period $::env(CLOCK_PERIOD) -name default
config_compile -pipeline_loops 0
csynth_design
close_project
exit
