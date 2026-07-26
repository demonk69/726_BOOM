if {![info exists ::env(BOOM_HLS_TOP)]} {
  error "BOOM_HLS_TOP must name a module synthesis top"
}

set top $::env(BOOM_HLS_TOP)
if {[info exists ::env(BOOM_HLS_GATE)]} {
  set gate $::env(BOOM_HLS_GATE)
} else {
  set gate gate3_3
}
set ::env(BOOM_HLS_PROJECT) boom_hls_${gate}_$top
set ::env(BOOM_HLS_SOLUTION) solution_module
set ::env(BOOM_HLS_CFLAGS_EXTRA) ""

source [file join [file dirname [info script]] create_project.tcl]
source [file join [file dirname [info script]] .. directives baseline_directives.tcl]
csynth_design
close_project
exit
