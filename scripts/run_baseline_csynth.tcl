if {[info exists ::env(BOOM_HLS_GATE)]} {
  set gate $::env(BOOM_HLS_GATE)
} else {
  set gate gate3_3
}
set ::env(BOOM_HLS_PROJECT) boom_hls_${gate}_baseline
set ::env(BOOM_HLS_SOLUTION) solution_baseline
set ::env(BOOM_HLS_TOP) boom_core_top
set ::env(BOOM_HLS_CFLAGS_EXTRA) ""

source [file join [file dirname [info script]] create_project.tcl]
source [file join [file dirname [info script]] .. directives baseline_directives.tcl]
csynth_design
close_project
exit
