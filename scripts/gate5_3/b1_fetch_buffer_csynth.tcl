set root $::env(HLS_BOOM_ROOT)
set project $::env(GATE5_3_B1_HLS_PROJECT)
set solution $::env(GATE5_3_B1_SOLUTION)
set depth $::env(GATE5_3_B1_DEPTH)
set defines "-DFETCH_BUFFER_DEPTH=$depth"
if {[info exists ::env(GATE5_3_B1_DEFINES)]} {
    append defines " " $::env(GATE5_3_B1_DEFINES)
}

set project_parent [file dirname $project]
set project_name [file tail $project]
file mkdir $project_parent
cd $project_parent
open_project -reset $project_name
set_top synth_fetch_buffer_top
add_files -cflags "-std=c++11 -I$root/include $defines" "$root/src/fetch_buffer.cpp"
add_files -cflags "-std=c++11 -I$root/include $defines" "$root/src/synth_fetch_buffer_top.cpp"
open_solution -reset $solution
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
config_compile -pipeline_loops 0
csynth_design
exit
