set root [file normalize $::env(HLS_BOOM_ROOT)]
set work [file normalize $::env(G6_L2_FOCUSED_WORK)]
set part [expr {[info exists ::env(FPGA_PART)] ? $::env(FPGA_PART) : "xczu7ev-ffvc1156-2-e"}]
set period [expr {[info exists ::env(CLOCK_PERIOD)] ? $::env(CLOCK_PERIOD) : 10}]
set lq_depth [expr {[info exists ::env(LQ_DEPTH)] ? $::env(LQ_DEPTH) : 8}]
set sq_depth [expr {[info exists ::env(SQ_DEPTH)] ? $::env(SQ_DEPTH) : 8}]
set cflags "-std=c++11 -DLQ_DEPTH=$lq_depth -DSQ_DEPTH=$sq_depth -I$root/include"
cd $work
open_project -reset hls_project
set_top g6_l2_focused_top
add_files -cflags $cflags [file join $root tb differential g6_l2_focused_top.cpp]
open_solution -reset solution
set_part $part
create_clock -period $period -name default
source [file join $root directives baseline_directives.tcl]
csynth_design
close_project
exit
