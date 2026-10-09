set dcp_path $::env(G6_L3_PPA3_DCP)
set out_dir $::env(G6_L3_PPA3_OUT)

file mkdir $out_dir

proc csv_quote {value} {
    set text [string map [list "\"" "\"\""] [format "%s" $value]]
    return "\"$text\""
}

proc safe_prop {object prop} {
    if {[catch {set value [get_property $prop $object]}]} {
        return ""
    }
    return $value
}

proc first_prop {object prop_list} {
    foreach prop $prop_list {
        set value [safe_prop $object $prop]
        if {$value ne ""} {
            return $value
        }
    }
    return ""
}

proc path_class {startpoint endpoint} {
    set text [string tolower "$startpoint $endpoint"]
    if {[regexp {ldq|load_queue|lq_.*owner|owner.*lq} $text]} {
        return "LSU_LQ_OWNER_SCAN"
    }
    if {[regexp {txid|transaction_id|next_transaction} $text]} {
        return "LSU_TXID_ALLOCATION"
    }
    if {[regexp {load_response|dmem_resp|response.*transaction|transaction.*response} $text]} {
        return "LOAD_RESPONSE_MATCHING"
    }
    if {[regexp {completion|writeback|wakeup} $text]} {
        return "COMPLETION_WRITEBACK"
    }
    if {[regexp {rob_commit|commit_module|grp_rob_commit} $text]} {
        return "ROB_COMMIT"
    }
    if {[regexp {frontend|predictor|ftq} $text]} {
        return "FRONTEND_PREDICTOR"
    }
    if {[regexp {reset_ctrl|reset_step|reset} $text]} {
        return "RESET_CONTROL"
    }
    return "OTHER"
}

proc module_hierarchy {startpoint endpoint} {
    foreach name [list $endpoint $startpoint] {
        set modules {}
        foreach part [split $name "/"] {
            if {[string match "grp_*" $part]} {
                lappend modules $part
            }
        }
        if {[llength $modules] > 0} {
            return [join $modules "/"]
        }
    }
    return [lindex [split $endpoint "/"] 0]
}

proc launch_fanout {startpoint} {
    set pins [get_pins -quiet $startpoint]
    if {[llength $pins] == 0} {
        return ""
    }
    set cells [get_cells -quiet -of_objects $pins]
    set max_fanout ""
    foreach cell $cells {
        foreach pin [get_pins -quiet -of_objects $cell -filter {DIRECTION == OUT}] {
            foreach net [get_nets -quiet -of_objects $pin] {
                set fanout [safe_prop $net FANOUT]
                if {$fanout ne "" && ($max_fanout eq "" || $fanout > $max_fanout)} {
                    set max_fanout $fanout
                }
            }
        }
    }
    return $max_fanout
}

proc run_report {name body} {
    global out_dir
    set status_path [file join $out_dir ppa3_audit_status.csv]
    set fh [open $status_path a]
    puts $fh "$name,START,"
    close $fh
    if {[catch {uplevel 1 $body} result]} {
        set fh [open $status_path a]
        puts $fh "$name,FAIL,[csv_quote $result]"
        close $fh
        return 0
    }
    set fh [open $status_path a]
    puts $fh "$name,PASS,"
    close $fh
    return 1
}

set sfh [open [file join $out_dir ppa3_audit_status.csv] w]
puts $sfh "stage,status,detail"
close $sfh

run_report open_checkpoint {
    open_checkpoint $dcp_path
}

run_report apply_6p450_clock {
    reset_timing
    create_clock -period 6.450 -name ap_clk [get_ports ap_clk]
}

run_report report_clocks {
    report_clocks -file [file join $out_dir report_clocks_6p450.rpt]
}

run_report check_timing {
    check_timing -verbose -file [file join $out_dir check_timing_6p450.rpt]
}

run_report report_exceptions {
    report_exceptions -file [file join $out_dir report_exceptions_6p450.rpt]
}

run_report report_methodology {
    report_methodology -file [file join $out_dir report_methodology_6p450.rpt]
}

run_report report_timing_summary {
    report_timing_summary -file [file join $out_dir report_timing_summary_6p450_100.rpt] -max_paths 100
}

run_report report_timing_100 {
    report_timing -file [file join $out_dir report_timing_6p450_100.rpt] -max_paths 100 -sort_by group -input_pins
}

run_report report_congestion {
    report_design_analysis -congestion -file [file join $out_dir report_congestion_6p450.rpt]
}

run_report report_design_analysis_timing {
    report_design_analysis -timing -setup -file [file join $out_dir report_design_analysis_timing_6p450.rpt]
}

run_report report_high_fanout_nets {
    report_high_fanout_nets -file [file join $out_dir report_high_fanout_nets_6p450.rpt] -timing -max_nets 200
}

run_report custom_high_fanout_csv {
    set rows {}
    foreach net [get_nets -hier] {
        set fanout [safe_prop $net FANOUT]
        if {$fanout ne ""} {
            lappend rows [list $fanout [safe_prop $net NAME]]
        }
    }
    set rows [lsort -integer -decreasing -index 0 $rows]
    set fh [open [file join $out_dir high_fanout_nets.csv] w]
    puts $fh "rank,fanout,net"
    set rank 0
    foreach row $rows {
        incr rank
        if {$rank > 200} {
            break
        }
        puts $fh "$rank,[lindex $row 0],[csv_quote [lindex $row 1]]"
    }
    close $fh
}

run_report timing_path_csv {
    set paths [get_timing_paths -setup -max_paths 100]
    if {[llength $paths] > 0} {
        set pfh [open [file join $out_dir timing_path_property_dump.txt] w]
        set first_path [lindex $paths 0]
        foreach prop [lsort [list_property $first_path]] {
            puts $pfh "$prop=[safe_prop $first_path $prop]"
        }
        close $pfh
    }

    set fh [open [file join $out_dir critical_paths_100.csv] w]
    puts $fh "rank,startpoint,endpoint,start_clock,end_clock,path_group,logic_levels,cell_delay_ns,net_delay_ns,data_path_delay_ns,clock_skew_ns,launch_fanout,slack_ns,module_hierarchy,path_class"
    set rank 0
    foreach path $paths {
        incr rank
        set startpoint [first_prop $path {STARTPOINT_PIN STARTPOINT}]
        set endpoint [first_prop $path {ENDPOINT_PIN ENDPOINT}]
        set start_clock [first_prop $path {STARTPOINT_CLOCK LAUNCH_CLOCK SOURCE_CLOCK}]
        set end_clock [first_prop $path {ENDPOINT_CLOCK CAPTURE_CLOCK DESTINATION_CLOCK}]
        set path_group [first_prop $path {PATH_GROUP GROUP}]
        set logic_levels [first_prop $path {LOGIC_LEVELS LOGIC_DEPTH}]
        set cell_delay [first_prop $path {DATAPATH_LOGIC_DELAY LOGIC_DELAY CELL_DELAY}]
        set net_delay [first_prop $path {DATAPATH_NET_DELAY NET_DELAY ROUTE_DELAY}]
        set data_delay [first_prop $path {DATAPATH_DELAY DATA_PATH_DELAY}]
        set skew [first_prop $path {SKEW CLOCK_SKEW}]
        set fanout [first_prop $path {MAX_FANOUT}]
        if {$fanout eq ""} {
            set fanout [launch_fanout $startpoint]
        }
        set slack [first_prop $path {SLACK}]
        set hierarchy [module_hierarchy $startpoint $endpoint]
        set class [path_class $startpoint $endpoint]
        puts $fh "$rank,[csv_quote $startpoint],[csv_quote $endpoint],[csv_quote $start_clock],[csv_quote $end_clock],[csv_quote $path_group],$logic_levels,$cell_delay,$net_delay,$data_delay,$skew,$fanout,$slack,[csv_quote $hierarchy],$class"
    }
    close $fh
}

exit
