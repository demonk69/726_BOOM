set dcp_path $::env(G6_L3_PPA5_PPA4_DCP)
set out_dir $::env(G6_L3_PPA5_AUDIT_OUT)
set status_path [file join $out_dir ppa4_netlist_audit_status.csv]

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

proc safe_property {prop obj} {
    if {[catch {get_property $prop $obj} value]} {
        return ""
    }
    return $value
}

set sfh [open $status_path w]
puts $sfh "stage,status,detail"
close $sfh

if {[catch {open_checkpoint $dcp_path} result]} {
    record_status open_checkpoint FAIL $result
    exit 1
}
record_status open_checkpoint PASS $dcp_path

set info_fh [open [file join $out_dir ppa4_design_identity.csv] w]
puts $info_fh "field,value"
puts $info_fh "checkpoint,[csv_quote $dcp_path]"
puts $info_fh "design,[csv_quote [current_design]]"
puts $info_fh "part,[csv_quote [safe_property PART [current_design]]]"
puts $info_fh "package,[csv_quote [safe_property PACKAGE [current_design]]]"
puts $info_fh "speed_grade,[csv_quote [safe_property SPEEDGRADE [current_design]]]"
close $info_fh

foreach {name body} {
    route_status {report_route_status -file [file join $out_dir ppa4_route_status_from_dcp.rpt]}
    timing_summary {report_timing_summary -file [file join $out_dir ppa4_timing_summary_from_dcp.rpt] -max_paths 100}
    timing {report_timing -file [file join $out_dir ppa4_timing_from_dcp.rpt] -max_paths 20 -sort_by group}
    utilization {report_utilization -file [file join $out_dir ppa4_utilization_from_dcp.rpt]}
    utilization_hier {report_utilization -hierarchical -hierarchical_depth 20 -file [file join $out_dir ppa4_hier_utilization_from_dcp.rpt]}
    io {report_io -file [file join $out_dir ppa4_io_from_dcp.rpt]}
    drc {report_drc -file [file join $out_dir ppa4_drc_from_dcp.rpt]}
    check_timing {check_timing -file [file join $out_dir ppa4_check_timing_from_dcp.rpt]}
} {
    if {[catch {eval $body} result]} {
        record_status $name FAIL $result
    } else {
        record_status $name PASS ""
    }
}

set cells [get_cells -hierarchical]

set inv_fh [open [file join $out_dir ppa4_hier_cell_inventory.csv] w]
puts $inv_fh "name,ref_name,is_primitive,is_sequential,is_blackbox"
foreach cell [lsort $cells] {
    set name [safe_property NAME $cell]
    set ref [safe_property REF_NAME $cell]
    set is_primitive [safe_property IS_PRIMITIVE $cell]
    set is_sequential [safe_property IS_SEQUENTIAL $cell]
    set is_blackbox [safe_property IS_BLACKBOX $cell]
    puts $inv_fh "[csv_quote $name],[csv_quote $ref],[csv_quote $is_primitive],[csv_quote $is_sequential],[csv_quote $is_blackbox]"
}
close $inv_fh

set bb_fh [open [file join $out_dir ppa4_black_boxes.csv] w]
puts $bb_fh "name,ref_name,is_blackbox"
set blackbox_count 0
foreach cell [lsort $cells] {
    set is_blackbox [safe_property IS_BLACKBOX $cell]
    if {$is_blackbox eq "1" || $is_blackbox eq "true" || $is_blackbox eq "TRUE"} {
        incr blackbox_count
        puts $bb_fh "[csv_quote [safe_property NAME $cell]],[csv_quote [safe_property REF_NAME $cell]],[csv_quote $is_blackbox]"
    }
}
close $bb_fh
record_status blackbox_count INFO $blackbox_count

set port_fh [open [file join $out_dir ppa4_top_ports.csv] w]
puts $port_fh "port,direction,width,net_count"
foreach port [lsort [get_ports *]] {
    set name [safe_property NAME $port]
    set dir [safe_property DIRECTION $port]
    set left [safe_property LEFT $port]
    set right [safe_property RIGHT $port]
    if {$left eq "" || $right eq ""} {
        set width 1
    } else {
        set width [expr {abs($left - $right) + 1}]
    }
    set nets [get_nets -quiet -of_objects $port]
    puts $port_fh "[csv_quote $name],[csv_quote $dir],$width,[llength $nets]"
}
close $port_fh

set categories {
    Frontend {frontend fetch predecode decompress_rvc build_fetch_packet}
    Predictor {predictor btb bpd branch_predict}
    FTQ {ftq}
    Decode {decode_module decode decompress_rvc predecode}
    Rename {rename_module rename free_list map_table}
    Issue {issue_module issue compact_issue_queue compact_iq iq_entries}
    Execute {execute_module execute alu divider mul}
    LSU {lsu load store dmem plan_load_issue finish_load_response}
    Completion {completion service_pending writeback}
    ROB {rob}
    Commit {commit rob_commit}
    CSR {csr}
    SystemAdapter {boom_l3_ppa4_memory_system memory_u ppa4}
    HlsCoreTop {boom_core_top core_u boom_core_cycle}
}

set mod_fh [open [file join $out_dir ppa4_major_module_presence.csv] w]
puts $mod_fh "category,patterns,total_matches,nonprimitive_matches,primitive_matches,sequential_matches,sample_matches"
foreach {category patterns} $categories {
    set total 0
    set nonprim 0
    set prim 0
    set seq 0
    set samples {}
    foreach cell $cells {
        set name_l [string tolower [safe_property NAME $cell]]
        set ref_l [string tolower [safe_property REF_NAME $cell]]
        set matched 0
        foreach pattern $patterns {
            set pattern_l [string tolower $pattern]
            if {[string first $pattern_l $name_l] >= 0 || [string first $pattern_l $ref_l] >= 0} {
                set matched 1
                break
            }
        }
        if {$matched} {
            incr total
            set is_primitive [safe_property IS_PRIMITIVE $cell]
            set is_sequential [safe_property IS_SEQUENTIAL $cell]
            if {$is_primitive eq "1" || $is_primitive eq "true" || $is_primitive eq "TRUE"} {
                incr prim
            } else {
                incr nonprim
            }
            if {$is_sequential eq "1" || $is_sequential eq "true" || $is_sequential eq "TRUE"} {
                incr seq
            }
            if {[llength $samples] < 8} {
                lappend samples [safe_property NAME $cell]
            }
        }
    }
    puts $mod_fh "[csv_quote $category],[csv_quote $patterns],$total,$nonprim,$prim,$seq,[csv_quote [join $samples {; }]]"
}
close $mod_fh

set net_fh [open [file join $out_dir ppa4_stream_connectivity_matches.csv] w]
puts $net_fh "stream,net_matches,pin_matches,sample_nets,sample_pins"
foreach stream {imem_req imem_resp dmem_req dmem_resp commit_trace io_success io_halted io_trap io_cycle io_instret} {
    set nets [get_nets -quiet -hierarchical *$stream*]
    set pins [get_pins -quiet -hierarchical *$stream*]
    set sample_nets {}
    set sample_pins {}
    foreach net $nets {
        if {[llength $sample_nets] < 8} {
            lappend sample_nets [safe_property NAME $net]
        }
    }
    foreach pin $pins {
        if {[llength $sample_pins] < 8} {
            lappend sample_pins [safe_property NAME $pin]
        }
    }
    puts $net_fh "[csv_quote $stream],[llength $nets],[llength $pins],[csv_quote [join $sample_nets {; }]],[csv_quote [join $sample_pins {; }]]"
}
close $net_fh

record_status complete DONE ""
exit
