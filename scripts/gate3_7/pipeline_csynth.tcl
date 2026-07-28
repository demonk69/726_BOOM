set root $::env(HLS_BOOM_ROOT)
set variant $::env(GATE37_VARIANT)
set project $::env(GATE37_PROJECT)
set directive $::env(GATE37_DIRECTIVE)
set part [expr {[info exists ::env(FPGA_PART)] ? $::env(FPGA_PART) : "xczu7ev-ffvc1156-2-e"}]
set clock [expr {[info exists ::env(CLOCK_PERIOD)] ? $::env(CLOCK_PERIOD) : "10"}]

exec "$root/scripts/generate_merged.sh"
file mkdir [file dirname $project]
cd [file dirname $project]
open_project -reset [file tail $project]
set_top boom_core_top
add_files -cflags "-std=c++11 -I$root/include" "$root/src/boom_core_merged.cpp"
open_solution -reset solution_pipeline
set_part $part
create_clock -period $clock -name default
config_compile -pipeline_loops 0
source $directive
csynth_design
close_project
exit
