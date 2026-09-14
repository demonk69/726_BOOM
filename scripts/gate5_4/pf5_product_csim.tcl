set root [file normalize $::env(HLS_BOOM_ROOT)]
set build [file normalize "$::env(BOOM_BUILD_ROOT)/product_csim"]
file mkdir $build
cd $build
exec "$root/scripts/generate_merged.sh"
open_project -reset pf5_product_csim
set_top boom_core_step
set cflags "-std=c++11 -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -DPF5_TRAINING_EXPECTED -I$root/include"
add_files -cflags $cflags "$root/src/boom_core_merged.cpp"
add_files -tb -cflags $cflags "$root/tb/differential/pf4_product_programs.cpp"
open_solution -reset solution_pf5_product
set_part xczu7ev-ffvc1156-2-e
create_clock -period 10 -name default
csim_design
close_project
exit
