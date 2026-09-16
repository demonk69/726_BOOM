set root [file normalize $::env(HLS_BOOM_ROOT)]
set work [file normalize $::env(G6_L1R80_FULL_CORE_WORK)]
set part [expr {[info exists ::env(FPGA_PART)] ? $::env(FPGA_PART) : "xczu7ev-ffvc1156-2-e"}]
set period [expr {[info exists ::env(CLOCK_PERIOD)] ? $::env(CLOCK_PERIOD) : 10}]
set cflags "-std=c++11 -I$root/include -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM"

cd $work
open_project -reset boom_hls_g6_l1r_default_8_8_boom_core_top
set_top boom_core_top
add_files -cflags $cflags [file join $root src boom_core_merged.cpp]
open_solution -reset solution_module
set_part $part
create_clock -period $period -name default
source [file join $root directives baseline_directives.tcl]
csynth_design
close_project
exit
