set dcp_path $::env(G6_L3_PPA2_DCP)
set out_dir $::env(G6_L3_PPA2_OUT)

file mkdir $out_dir
open_checkpoint $dcp_path
reset_timing
create_clock -period 6.450 -name ap_clk [get_ports ap_clk]
report_timing_summary -file [file join $out_dir ooc_route_timing_summary_6p450.rpt] -max_paths 10
report_timing -file [file join $out_dir ooc_route_critical_path_6p450.rpt] -max_paths 10 -sort_by group
exit
