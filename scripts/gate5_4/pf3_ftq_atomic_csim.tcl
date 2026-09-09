set root [file normalize [expr {[info exists ::env(HLS_BOOM_ROOT)] ? $::env(HLS_BOOM_ROOT) : [file join [file dirname [info script]] ../..]}]]
set build [file normalize [expr {[info exists ::env(BOOM_BUILD_ROOT)] ? "$::env(BOOM_BUILD_ROOT)/gate5_4_product_integration/pf3/csim" : "/tmp/boom_hls/gate5_4_product_integration/pf3/csim"}]]
file mkdir $build
cd $build
exec "$root/scripts/generate_merged.sh"
open_project -reset pf3_ftq_atomic_csim
set_top synth_pf3_ftq_atomic_top
set cflags "-std=c++11 -DBOOM_FTQ_STORAGE_LUTRAM -I$root/include"
add_files -cflags $cflags "$root/src/boom_core_merged.cpp"
add_files -tb -cflags $cflags "$root/tb/differential/pf3_ftq_atomic_tests.cpp"
open_solution -reset solution_pf3_csim
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csim_design
close_project
exit
