set root [file normalize [file join [file dirname [info script]] ../..]]
if {[info exists ::env(HLS_BOOM_ROOT)]} {
    set root [file normalize $::env(HLS_BOOM_ROOT)]
}
if {![info exists ::env(GATE5_2_R2_HLS_PROJECT)]} {
    error "GATE5_2_R2_HLS_PROJECT must name the R2 HLS project"
}

set project [file normalize $::env(GATE5_2_R2_HLS_PROJECT)]
file mkdir [file dirname $project]
cd [file dirname $project]
open_project -reset [file tail $project]
set_top synth_r2_rvc_frontend_top
set cflags "-std=c++11 -I$root/include"
add_files -cflags $cflags $root/src/frontend.cpp
add_files -cflags $cflags $root/src/rvc.cpp
add_files -cflags $cflags $root/src/decode.cpp
add_files -cflags $cflags $root/src/divider.cpp
add_files -cflags $cflags $root/src/synth_module_tops.cpp
open_solution -reset solution_r2_rvc_frontend
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csynth_design
close_project
exit
