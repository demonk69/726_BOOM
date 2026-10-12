set repo_root $::env(G6_L3_PPA4_REPO_ROOT)
set rtl_dir $::env(G6_L3_PPA4_RTL_DIR)
set out_dir $::env(G6_L3_PPA4_ELAB_OUT)
set status_path [file join $out_dir ppa4_elab_status.csv]
set part_name "xczu7ev-ffvc1156-2-e"

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
    create_project -force ppa4_system_elab [file join $out_dir vivado_project] -part $part_name
    foreach dat_file [glob -nocomplain -directory $rtl_dir *.dat] {
        file copy -force $dat_file $out_dir
    }
    read_verilog [lsort [glob -nocomplain -directory $rtl_dir *.v]]
    read_verilog [file join $repo_root reports/gate6_0_full_lsu/l3/system_integration/rtl/boom_l3_ppa4_memory_system.v]
    read_verilog [file join $repo_root reports/gate6_0_full_lsu/l3/system_integration/rtl/boom_l3_ppa4_system_top.v]
    set_property top boom_l3_ppa4_system_top [current_fileset]
}]

if {$setup_ok} {
    set elab_ok [run_stage elaboration {
        synth_design -rtl -top boom_l3_ppa4_system_top -part $part_name
        report_io -file [file join $out_dir ppa4_elab_io.rpt]
    }]
} else {
    record_status elaboration NOT_RUN "setup did not pass"
}

record_status complete DONE ""
exit
