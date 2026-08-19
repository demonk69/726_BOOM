set root [file normalize [file join [file dirname [info script]] ../..]]
set project [file normalize "$root/build/gate5_3_fetch_buffer/b3i/fetch_packet_hls"]
file mkdir [file dirname $project]
cd [file dirname $project]
open_project -reset [file tail $project]
set_top synth_fetch_packet_top
set cflags "-std=c++11 -I$root/include"
add_files -cflags $cflags $root/src/fetch_packet.cpp
add_files -cflags $cflags $root/src/rvc.cpp
add_files -cflags $cflags $root/src/synth_module_tops.cpp
open_solution -reset solution_b3i_packet
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
config_compile -pipeline_loops 0
csynth_design
close_project
exit
