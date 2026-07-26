set script_dir [file dirname [info script]]
set root [file normalize [file join $script_dir ..]]

if {[info exists ::env(BOOM_HLS_GATE)]} {
  set gate $::env(BOOM_HLS_GATE)
} else {
  set gate gate3_3
}

if {[info exists ::env(BOOM_HLS_PROJECT)]} {
  set project $::env(BOOM_HLS_PROJECT)
} else {
  set project boom_hls_$gate
}

if {[info exists ::env(BOOM_HLS_SOLUTION)]} {
  set solution $::env(BOOM_HLS_SOLUTION)
} else {
  set solution solution_baseline
}

if {[info exists ::env(BOOM_HLS_TOP)]} {
  set top $::env(BOOM_HLS_TOP)
} else {
  set top boom_core_top
}

if {[info exists ::env(FPGA_PART)]} {
  set part $::env(FPGA_PART)
} else {
  set part xczu7ev-ffvc1156-2-e
}

if {[info exists ::env(CLOCK_PERIOD)]} {
  set clk $::env(CLOCK_PERIOD)
} else {
  set clk 10
}

if {[info exists ::env(BOOM_HLS_CFLAGS_EXTRA)]} {
  set cflags "-std=c++11 -I$root/include $::env(BOOM_HLS_CFLAGS_EXTRA)"
} else {
  set cflags "-std=c++11 -I$root/include"
}

exec $root/scripts/generate_merged.sh

open_project -reset $project
set_top $top
add_files -cflags $cflags $root/src/boom_core_merged.cpp
open_solution -reset $solution
set_part $part
create_clock -period $clk -name default
