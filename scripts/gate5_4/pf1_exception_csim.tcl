set root [file normalize [expr {[info exists ::env(HLS_BOOM_ROOT)] ? $::env(HLS_BOOM_ROOT) : [file join [file dirname [info script]] ../..]}]]
set build [file normalize [expr {[info exists ::env(BOOM_BUILD_ROOT)] ? "$::env(BOOM_BUILD_ROOT)/gate5_4_product_integration/pf1/csim" : "/tmp/boom_hls/gate5_4_product_integration/pf1/csim"}]]
file mkdir $build
cd $build
open_project -reset pf1_exception_csim
set_top boom_core_step
set cflags "-std=c++11 -I$root/include"
foreach source {frontend fetch_packet fetch_buffer predecode predictor decode rename issue execute branch lsu commit csr completion rob reset divider mul rvc boom_core_step} {
    add_files -cflags $cflags "$root/src/$source.cpp"
}
add_files -tb -cflags $cflags "$root/tb/differential/exception_recovery_program_tests.cpp"
open_solution -reset solution_pf1_csim
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csim_design
close_project
exit
