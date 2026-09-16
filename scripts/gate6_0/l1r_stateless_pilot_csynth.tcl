set root [file normalize $::env(HLS_BOOM_ROOT)]
set work [file normalize $::env(G6_L1R_PILOT_WORK)]
set top $::env(G6_L1R_PILOT_TOP)
set part [expr {[info exists ::env(FPGA_PART)] ? $::env(FPGA_PART) : "xczu7ev-ffvc1156-2-e"}]
set period [expr {[info exists ::env(CLOCK_PERIOD)] ? $::env(CLOCK_PERIOD) : 10}]
set lq_depth [expr {[info exists ::env(LQ_DEPTH)] ? $::env(LQ_DEPTH) : 8}]
set sq_depth [expr {[info exists ::env(SQ_DEPTH)] ? $::env(SQ_DEPTH) : 8}]
set wrapper [expr {[info exists ::env(G6_L1R_PILOT_WRAPPER)] ? $::env(G6_L1R_PILOT_WRAPPER) : "tb/differential/g6_l1r_stateless_pilot_tops.cpp"}]
set product [expr {[info exists ::env(G6_L1R_PILOT_PRODUCT)] ? $::env(G6_L1R_PILOT_PRODUCT) : "merged"}]
set cflags "-std=c++11 -DLQ_DEPTH=$lq_depth -DSQ_DEPTH=$sq_depth -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I$root/include"

cd $work
open_project -reset hls_project
set_top $top
if {$product == "merged"} {
    add_files -cflags $cflags [file join $root src boom_core_merged.cpp]
}
add_files -cflags $cflags [file join $root $wrapper]
open_solution -reset solution
set_part $part
create_clock -period $period -name default
csynth_design
close_project
exit
