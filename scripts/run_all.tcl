# run_all.tcl - Complete HLS flow
# Usage: vitis_hls -f run_all.tcl
# Environment: FPGA_PART, CLOCK_PERIOD

set part  [expr {$::env(FPGA_PART)}]
set clk   [expr {$::env(CLOCK_PERIOD)}]

puts "=== BOOM HLS Full Flow ==="
puts "  FPGA Part:     $part"
puts "  Clock Period:  $clk ns"

# --- CSIM ---
puts "\n--- Stage 1: C Simulation ---"
open_project boom_hls_full

set_top boom_core_top

add_files -cflags "-std=c++11 -I../include" ../src/boom_core_top.cpp
add_files -cflags "-std=c++11 -I../include" ../src/boom_core_step.cpp
add_files -cflags "-std=c++11 -I../include" ../src/reset.cpp
add_files -cflags "-std=c++11 -I../include" ../src/frontend.cpp
add_files -cflags "-std=c++11 -I../include" ../src/decode.cpp
add_files -cflags "-std=c++11 -I../include" ../src/rename.cpp
add_files -cflags "-std=c++11 -I../include" ../src/rob.cpp
add_files -cflags "-std=c++11 -I../include" ../src/issue.cpp
add_files -cflags "-std=c++11 -I../include" ../src/execute.cpp
add_files -cflags "-std=c++11 -I../include" ../src/branch.cpp
add_files -cflags "-std=c++11 -I../include" ../src/lsu.cpp
add_files -cflags "-std=c++11 -I../include" ../src/commit.cpp
add_files -cflags "-std=c++11 -I../include" ../src/csr.cpp

add_files -tb -cflags "-std=c++11 -I../include" ../tb/tb_boom_core.cpp

open_solution "solution_full"
set_part $part
create_clock -period $clk -name default

csim_design

# --- CSYNTH ---
puts "\n--- Stage 2: C Synthesis (baseline) ---"
csynth_design

# --- COSIM ---
puts "\n--- Stage 3: C/RTL Co-simulation ---"
cosim_design

# --- EXPORT ---
puts "\n--- Stage 4: Export RTL ---"
export_design -rtl verilog -format ip_catalog

close_project

puts "\n=== BOOM HLS Full Flow Complete ==="
