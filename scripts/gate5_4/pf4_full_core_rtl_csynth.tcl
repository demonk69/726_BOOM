set root [file normalize [file join [file dirname [info script]] ../..]]
if {![info exists ::env(GATE5_4_PF4_RTL_HLS_PROJECT)]} {
    error "GATE5_4_PF4_RTL_HLS_PROJECT is required"
}
set project [file normalize $::env(GATE5_4_PF4_RTL_HLS_PROJECT)]
file mkdir [file dirname $project]
cd [file dirname $project]
open_project -reset [file tail $project]
set_top boom_core_pf4_rtl_top
set cflags "-std=c++11 -I$root/include -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM"
add_files -cflags $cflags $root/src/boom_core_merged.cpp
open_solution -reset solution_pf4_rtl
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
source $root/directives/baseline_directives.tcl
csynth_design
close_project
exit
