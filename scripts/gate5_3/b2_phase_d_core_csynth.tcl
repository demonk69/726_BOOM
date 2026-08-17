set root [file normalize [file join [file dirname [info script]] ../..]]
if {![info exists ::env(GATE5_3_B2_D_HLS_PROJECT)]} {
    error "GATE5_3_B2_D_HLS_PROJECT is required"
}

set project [file normalize $::env(GATE5_3_B2_D_HLS_PROJECT)]
file mkdir [file dirname $project]
cd [file dirname $project]
open_project -reset [file tail $project]
set_top boom_core_top
set cflags "-std=c++11 -I$root/include"
add_files -cflags $cflags $root/src/boom_core_merged.cpp
open_solution -reset solution_phase_d
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
source $root/directives/baseline_directives.tcl
csynth_design
close_project
exit
