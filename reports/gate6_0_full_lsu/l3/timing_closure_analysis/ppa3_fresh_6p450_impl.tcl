set synth_dcp $::env(G6_L3_PPA3_SYNTH_DCP)
set out_dir $::env(G6_L3_PPA3_FRESH_OUT)

file mkdir $out_dir

proc csv_quote {value} {
    set text [string map [list "\"" "\"\""] [format "%s" $value]]
    return "\"$text\""
}

proc record_status {stage status detail} {
    global out_dir
    set fh [open [file join $out_dir fresh_6p450_status.csv] a]
    puts $fh "$stage,$status,[csv_quote $detail]"
    close $fh
}

proc run_stage {stage body} {
    record_status $stage START ""
    if {[catch {uplevel 1 $body} result]} {
        record_status $stage FAIL $result
        return 0
    }
    record_status $stage PASS ""
    return 1
}

set sfh [open [file join $out_dir fresh_6p450_status.csv] w]
puts $sfh "stage,status,detail"
close $sfh

set open_ok [run_stage open_synth_checkpoint {
    open_checkpoint $synth_dcp
    reset_timing
    create_clock -period 6.450 -name ap_clk [get_ports ap_clk]
    report_clocks -file [file join $out_dir fresh_report_clocks_6p450.rpt]
    check_timing -verbose -file [file join $out_dir fresh_check_timing_6p450.rpt]
}]

if {$open_ok} {
    set opt_ok [run_stage opt_design {
        opt_design
        report_utilization -file [file join $out_dir fresh_post_opt_utilization.rpt]
        report_timing_summary -file [file join $out_dir fresh_post_opt_timing_summary.rpt] -max_paths 20
        write_checkpoint -force [file join $out_dir fresh_post_opt.dcp]
    }]
} else {
    set opt_ok 0
    record_status opt_design NOT_RUN "open_synth_checkpoint did not pass"
}

if {$opt_ok} {
    set place_ok [run_stage place_design {
        place_design
        report_utilization -file [file join $out_dir fresh_post_place_utilization.rpt]
        report_timing_summary -file [file join $out_dir fresh_post_place_timing_summary.rpt] -max_paths 20
        report_design_analysis -congestion -file [file join $out_dir fresh_post_place_congestion.rpt]
        report_drc -file [file join $out_dir fresh_post_place_drc.rpt]
        write_checkpoint -force [file join $out_dir fresh_post_place.dcp]
    }]
} else {
    set place_ok 0
    record_status place_design NOT_RUN "opt_design did not pass"
}

if {$place_ok} {
    set phys_ok [run_stage phys_opt_design {
        phys_opt_design
        report_utilization -file [file join $out_dir fresh_post_phys_opt_utilization.rpt]
        report_timing_summary -file [file join $out_dir fresh_post_phys_opt_timing_summary.rpt] -max_paths 20
        report_design_analysis -congestion -file [file join $out_dir fresh_post_phys_opt_congestion.rpt]
        report_drc -file [file join $out_dir fresh_post_phys_opt_drc.rpt]
        write_checkpoint -force [file join $out_dir fresh_post_phys_opt.dcp]
    }]
} else {
    set phys_ok 0
    record_status phys_opt_design NOT_RUN "place_design did not pass"
}

if {$phys_ok} {
    set route_ok [run_stage route_design {
        route_design
        report_route_status -file [file join $out_dir fresh_route_status.rpt]
        report_utilization -file [file join $out_dir fresh_post_route_utilization.rpt]
        report_timing_summary -file [file join $out_dir fresh_post_route_timing_summary.rpt] -max_paths 100
        report_timing -file [file join $out_dir fresh_post_route_critical_path.rpt] -max_paths 20 -sort_by group
        report_design_analysis -congestion -file [file join $out_dir fresh_post_route_congestion.rpt]
        report_design_analysis -timing -setup -file [file join $out_dir fresh_post_route_design_analysis_timing.rpt]
        report_drc -file [file join $out_dir fresh_post_route_drc.rpt]
        write_checkpoint -force [file join $out_dir fresh_post_route.dcp]
    }]
} else {
    set route_ok 0
    record_status route_design NOT_RUN "phys_opt_design did not pass"
}

record_status implementation COMPLETE ""
exit
