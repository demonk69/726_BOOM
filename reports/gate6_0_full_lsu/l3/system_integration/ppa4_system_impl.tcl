set out_dir $::env(G6_L3_PPA4_IMPL_OUT)
set synth_dcp $::env(G6_L3_PPA4_SYNTH_DCP)
set status_path [file join $out_dir ppa4_impl_status.csv]

file mkdir $out_dir
cd $out_dir

proc csv_quote {value} {
    set text [string map [list "\"" "\"\""] [format "%s" $value]]
    return "\"$text\""
}

proc record_status {stage status detail} {
    global status_path
    set fh [open $status_path a]
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

set sfh [open $status_path w]
puts $sfh "stage,status,detail"
close $sfh

set setup_ok [run_stage setup {
    open_checkpoint $synth_dcp
}]

if {$setup_ok} {
    set opt_ok [run_stage opt {
        opt_design
        report_utilization -file [file join $out_dir ppa4_post_opt_utilization.rpt]
        report_timing_summary -file [file join $out_dir ppa4_post_opt_timing_summary.rpt] -max_paths 20
        write_checkpoint -force [file join $out_dir ppa4_post_opt.dcp]
    }]
} else {
    record_status opt NOT_RUN "setup did not pass"
    set opt_ok 0
}

if {$opt_ok} {
    set place_ok [run_stage place {
        place_design
        report_io -file [file join $out_dir ppa4_post_place_io.rpt]
        report_utilization -file [file join $out_dir ppa4_post_place_utilization.rpt]
        report_timing_summary -file [file join $out_dir ppa4_post_place_timing_summary.rpt] -max_paths 20
        report_design_analysis -congestion -file [file join $out_dir ppa4_post_place_congestion.rpt]
        report_drc -file [file join $out_dir ppa4_post_place_drc.rpt]
        write_checkpoint -force [file join $out_dir ppa4_post_place.dcp]
    }]
} else {
    record_status place NOT_RUN "opt did not pass"
    set place_ok 0
}

if {$place_ok} {
    set phys_opt_ok [run_stage phys_opt {
        phys_opt_design
        report_utilization -file [file join $out_dir ppa4_post_phys_opt_utilization.rpt]
        report_timing_summary -file [file join $out_dir ppa4_post_phys_opt_timing_summary.rpt] -max_paths 20
        report_design_analysis -congestion -file [file join $out_dir ppa4_post_phys_opt_congestion.rpt]
        report_drc -file [file join $out_dir ppa4_post_phys_opt_drc.rpt]
        write_checkpoint -force [file join $out_dir ppa4_post_phys_opt.dcp]
    }]
} else {
    record_status phys_opt NOT_RUN "place did not pass"
    set phys_opt_ok 0
}

if {$phys_opt_ok} {
    set route_ok [run_stage route {
        route_design
        report_route_status -file [file join $out_dir ppa4_post_route_status.rpt]
        report_io -file [file join $out_dir ppa4_post_route_io.rpt]
        report_utilization -file [file join $out_dir ppa4_post_route_utilization.rpt]
        report_timing_summary -file [file join $out_dir ppa4_post_route_timing_summary.rpt] -max_paths 100
        report_timing -file [file join $out_dir ppa4_post_route_timing.rpt] -max_paths 20 -sort_by group
        report_design_analysis -congestion -file [file join $out_dir ppa4_post_route_congestion.rpt]
        report_design_analysis -timing -setup -file [file join $out_dir ppa4_post_route_timing_analysis.rpt]
        report_drc -file [file join $out_dir ppa4_post_route_drc.rpt]
        write_checkpoint -force [file join $out_dir ppa4_post_route.dcp]
    }]
} else {
    record_status route NOT_RUN "phys_opt did not pass"
}

record_status complete DONE ""
exit
