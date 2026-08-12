set root $::env(HLS_PROJECT_ROOT)
open_project -reset r2_boom_core_top
set_top boom_core_top
foreach source {frontend decode rename issue execute branch lsu commit csr completion rob reset divider mul rvc boom_core_step boom_core_top} {
  add_files -cflags "-std=c++11 -I$root/include" "$root/src/$source.cpp"
}
open_solution -reset solution_r2_rtl
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csynth_design
close_project
exit
