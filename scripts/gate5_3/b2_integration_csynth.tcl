set root [file normalize [file join [file dirname [info script]] ../..]]
set project [file normalize "$root/build/gate5_3_fetch_buffer/b2/rtl_hls"]
file mkdir [file dirname $project]
cd [file dirname $project]
open_project -reset [file tail $project]
set_top synth_fetch_buffer_integration_top
set cflags "-std=c++11 -I$root/include"
add_files -cflags $cflags $root/src/boom_core_merged.cpp
open_solution -reset solution_b2_integration
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csynth_design
close_project
exit
