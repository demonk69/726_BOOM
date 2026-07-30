set root [file normalize [file join [file dirname [info script]] ../..]]
set variant $::env(GATE3_10_VARIANT)
set project hls_project
set solution solution_local
set cflags "-std=c++11 -I$root/include"
if {[info exists ::env(BOOM_HLS_CFLAGS_EXTRA)]} {
  append cflags " $::env(BOOM_HLS_CFLAGS_EXTRA)"
}
exec $root/scripts/generate_merged.sh
open_project -reset $project
set_top boom_core_top
add_files -cflags $cflags $root/src/boom_core_merged.cpp
open_solution -reset $solution
set_part $::env(FPGA_PART)
create_clock -period $::env(CLOCK_PERIOD) -name default
config_compile -pipeline_loops 0
source [file join $root directives baseline_directives.tcl]
csynth_design
close_project
exit
