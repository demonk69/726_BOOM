if {![info exists ::env(BOOM_HLS_TOP)]} {
  error "BOOM_HLS_TOP must name a Gate 3.6 N-cycle synthesis top"
}

set top $::env(BOOM_HLS_TOP)
set ::env(BOOM_HLS_GATE) gate3_6
set ::env(BOOM_HLS_PROJECT) boom_hls_gate3_6_$top
set ::env(BOOM_HLS_SOLUTION) solution_ncycle
set ::env(BOOM_HLS_CFLAGS_EXTRA) ""

source [file join [file dirname [info script]] .. create_project.tcl]
source [file join [file dirname [info script]] .. .. directives baseline_directives.tcl]
csynth_design
close_project
exit
