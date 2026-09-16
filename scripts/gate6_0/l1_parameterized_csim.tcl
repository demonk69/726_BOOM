set root [file normalize [expr {[info exists ::env(HLS_BOOM_ROOT)] ? $::env(HLS_BOOM_ROOT) : [file join [file dirname [info script]] ../..]}]]
set build_root [file normalize [expr {[info exists ::env(BOOM_BUILD_ROOT)] ? $::env(BOOM_BUILD_ROOT) : "/tmp/boom_hls/g6_l1/csim"}]]
set lq_depth [expr {[info exists ::env(LQ_DEPTH)] ? $::env(LQ_DEPTH) : 8}]
set sq_depth [expr {[info exists ::env(SQ_DEPTH)] ? $::env(SQ_DEPTH) : 8}]
set config "${lq_depth}_${sq_depth}"
set build "$build_root/$config"
file mkdir $build
cd $build
open_project -reset "g6_l1_csim_$config"
set_top boom_core_step
set cflags "-std=c++11 -I$root/include -DLQ_DEPTH=$lq_depth -DSQ_DEPTH=$sq_depth"
add_files -cflags $cflags "$root/src/boom_core_merged.cpp"
add_files -tb -cflags $cflags "$root/tb/differential/g6_l1_parameterized_lq_sq_tests.cpp"
open_solution -reset "solution_$config"
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csim_design
close_project
exit
