set repo_root $::env(G6_L3_PPA4_REPO_ROOT)
set out_dir $::env(G6_L3_PPA4_SIM_OUT)
set part_name "xczu7ev-ffvc1156-2-e"

file mkdir $out_dir
cd $out_dir

create_project -force ppa4_adapter_scoreboard [file join $out_dir vivado_project] -part $part_name
read_verilog [file join $repo_root reports/gate6_0_full_lsu/l3/system_integration/rtl/boom_l3_ppa4_memory_system.v]
read_verilog -sv [file join $repo_root reports/gate6_0_full_lsu/l3/system_integration/tb/ppa4_memory_system_scoreboard_tb.sv]
set_property top ppa4_memory_system_scoreboard_tb [get_filesets sim_1]
set_property xsim.simulate.runtime all [get_filesets sim_1]
launch_simulation -simset sim_1 -mode behavioral
exit
