set dcp_path $::env(G6_L3_PPA2_DCP)
set out_dir $::env(G6_L3_PPA2_OUT)

file mkdir $out_dir
open_checkpoint $dcp_path

set csv_path [file join $out_dir synth_ports.csv]
set fh [open $csv_path w]
puts $fh "name,direction,width"
foreach p [lsort [get_ports]] {
    set dir [get_property DIRECTION $p]
    set left [get_property LEFT $p]
    set right [get_property RIGHT $p]
    if {$left eq "" || $right eq ""} {
        set width 1
    } else {
        set width [expr {abs($left - $right) + 1}]
    }
    puts $fh "$p,$dir,$width"
}
close $fh

report_io -file [file join $out_dir report_io.rpt]

set iob_path [file join $out_dir synth_iob_cells.csv]
set ifh [open $iob_path w]
puts $ifh "cell,ref_name"
foreach c [lsort [get_cells -hier -filter {REF_NAME == IBUF || REF_NAME == INBUF || REF_NAME == OBUF}]] {
    puts $ifh "$c,[get_property REF_NAME $c]"
}
close $ifh
exit
