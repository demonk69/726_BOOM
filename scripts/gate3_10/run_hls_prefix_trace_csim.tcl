set root $::env(HLS_PROJECT_ROOT)
set cflags "-std=c++11 -I$root/include"
if {[info exists ::env(BOOM_HLS_CFLAGS_EXTRA)]} {
  append cflags " $::env(BOOM_HLS_CFLAGS_EXTRA)"
}
open_project -reset $::env(GATE3_10_CSIM_PROJECT)
set_top boom_core_top
foreach source {boom_core_top.cpp boom_core_step.cpp reset.cpp frontend.cpp decode.cpp rename.cpp rob.cpp issue.cpp execute.cpp branch.cpp lsu.cpp commit.cpp csr.cpp} {
  add_files -cflags $cflags "$root/src/$source"
}
add_files -tb -cflags $cflags "$root/tb/differential/hls_prefix_trace_tb.cpp"
open_solution solution_prefix_trace
set_part $::env(FPGA_PART)
create_clock -period $::env(CLOCK_PERIOD) -name default
csim_design
close_project
exit
