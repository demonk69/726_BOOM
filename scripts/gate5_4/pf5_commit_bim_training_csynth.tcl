set root [file normalize $::env(HLS_BOOM_ROOT)]
set work [file normalize $::env(PF5_WORK)]
set part [expr {[info exists ::env(FPGA_PART)] ? $::env(FPGA_PART) : "xczu7ev-ffvc1156-2-e"}]
set period [expr {[info exists ::env(CLOCK_PERIOD)] ? $::env(CLOCK_PERIOD) : 10}]
set cflags "-std=c++11 -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I$root/include"

cd $work
open_project -reset hls_project
set_top synth_pf5_commit_bim_training_top
add_files -cflags $cflags [file join $root src boom_core_merged.cpp]
open_solution -reset solution
set_part $part
create_clock -period $period -name default
csynth_design
close_project
exit
