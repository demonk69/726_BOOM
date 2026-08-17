set root [file normalize [file join [file dirname [info script]] ../..]]
set build [file normalize "$root/build/gate5_3_fetch_buffer/b2/csim"]
file mkdir $build
cd $build
open_project -reset b2_full_core_csim
set_top boom_core_step
set cflags "-std=c++11 -I$root/include"
foreach source {frontend fetch_buffer decode rename issue execute branch lsu commit csr completion rob reset divider mul rvc boom_core_step} {
    add_files -cflags $cflags "$root/src/$source.cpp"
}
add_files -tb -cflags $cflags $root/tb/differential/gate5_2_r2_full_core_rvc.cpp
open_solution -reset solution_b2_csim
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csim_design
close_project
exit
