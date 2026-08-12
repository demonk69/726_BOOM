set root $::env(HLS_PROJECT_ROOT)
open_project -reset r2_full_core_rvc_csim
set_top boom_core_step
foreach source {frontend decode rename issue execute branch lsu commit csr completion rob reset divider mul rvc boom_core_step} {
  add_files -cflags "-std=c++11 -I$root/include" "$root/src/$source.cpp"
}
add_files -tb -cflags "-std=c++11 -I$root/include" "$root/tb/differential/gate5_2_r2_full_core_rvc.cpp"
open_solution -reset solution_r2_csim
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csim_design
close_project
exit
