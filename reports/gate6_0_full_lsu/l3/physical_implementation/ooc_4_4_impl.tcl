set rtl_dir $::env(G6_L3_PPA2_RTL_DIR)
set work_dir $::env(G6_L3_PPA2_WORK)
set status_path $::env(G6_L3_PPA2_STATUS)
set part_name "xczu7ev-ffvc1156-2-e"

file mkdir $work_dir
cd $work_dir

proc record_status {stage status detail} {
    global status_path
    set fh [open $status_path a]
    puts $fh "$stage,$status,$detail"
    close $fh
}

proc run_stage {stage body} {
    record_status $stage START ""
    if {[catch {uplevel 1 $body} result options]} {
        record_status $stage FAIL $result
        return 0
    }
    record_status $stage PASS ""
    return 1
}

proc write_iob_count {path} {
    set fh [open $path w]
    puts $fh "ref_name,count"
    foreach ref {IBUF INBUF OBUF IBUFCTRL} {
        puts $fh "$ref,[llength [get_cells -hier -filter [format {REF_NAME == %s} $ref]]]"
    }
    close $fh
}

set sfh [open $status_path w]
puts $sfh "stage,status,detail"
close $sfh

set setup_ok [run_stage setup {
    create_project -force g6_l3_4_4_ooc [file join $work_dir vivado_project] -part $part_name
    foreach dat_file [glob -nocomplain -directory $rtl_dir *.dat] {
        file copy -force $dat_file $work_dir
    }
    set rtl_files [lsort [glob -nocomplain -directory $rtl_dir *.v]]
    read_verilog $rtl_files
    set_property top boom_core_top [current_fileset]
    set xdc_path [file join $work_dir g6_l3_4_4_ooc_clock.xdc]
    set xfh [open $xdc_path w]
    puts $xfh {create_clock -period 10.000 -name ap_clk [get_ports ap_clk]}
    close $xfh
    read_xdc $xdc_path
}]

if {$setup_ok} {
    set synth_ok [run_stage synth_design_ooc {
        synth_design -top boom_core_top -part $part_name -mode out_of_context
        report_utilization -file [file join $work_dir ooc_synth_utilization.rpt]
        report_timing_summary -file [file join $work_dir ooc_synth_timing_summary.rpt] -max_paths 10
        report_io -file [file join $work_dir ooc_synth_io.rpt]
        write_iob_count [file join $work_dir ooc_synth_iob_counts.csv]
        write_checkpoint -force [file join $work_dir ooc_post_synth.dcp]
    }]
} else {
    set synth_ok 0
}

if {$synth_ok} {
    set opt_ok [run_stage opt_design {
        opt_design
        write_iob_count [file join $work_dir ooc_opt_iob_counts.csv]
        write_checkpoint -force [file join $work_dir ooc_post_opt.dcp]
    }]
} else {
    set opt_ok 0
    record_status opt_design NOT_RUN "synth_design_ooc did not pass"
}

if {$opt_ok} {
    set place_ok [run_stage place_design {
        place_design
        report_utilization -file [file join $work_dir ooc_place_utilization.rpt]
        report_timing_summary -file [file join $work_dir ooc_place_timing_summary.rpt] -max_paths 10
        report_design_analysis -congestion -file [file join $work_dir ooc_place_congestion.rpt]
        report_drc -file [file join $work_dir ooc_place_drc.rpt]
        write_iob_count [file join $work_dir ooc_place_iob_counts.csv]
        write_checkpoint -force [file join $work_dir ooc_post_place.dcp]
    }]
} else {
    set place_ok 0
    record_status place_design NOT_RUN "opt_design did not pass"
}

if {$place_ok} {
    set route_ok [run_stage route_design {
        route_design
        report_route_status -file [file join $work_dir ooc_route_status.rpt]
        report_utilization -file [file join $work_dir ooc_route_utilization.rpt]
        report_timing_summary -file [file join $work_dir ooc_route_timing_summary.rpt] -max_paths 10
        report_timing -file [file join $work_dir ooc_route_critical_path.rpt] -max_paths 10 -sort_by group
        report_design_analysis -congestion -file [file join $work_dir ooc_route_congestion.rpt]
        report_drc -file [file join $work_dir ooc_route_drc.rpt]
        write_iob_count [file join $work_dir ooc_route_iob_counts.csv]
        write_checkpoint -force [file join $work_dir ooc_post_route.dcp]
    }]
} else {
    set route_ok 0
    record_status route_design NOT_RUN "place_design did not pass"
}

record_status implementation COMPLETE ""
exit
