// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2021.2 (64-bit)
// Copyright 1986-2021 Xilinx, Inc. All Rights Reserved.
// ==============================================================
`timescale 1 ns / 1 ps
module boom_core_top_boom_core_cycle_io_state_rob_entries_uop_exc_cause_RAM_AUTO_0R0W (address0, ce0, d0, we0,  reset,clk);

parameter DataWidth = 64;
parameter AddressWidth = 5;
parameter AddressRange = 32;

input[AddressWidth-1:0] address0;
input ce0;
input[DataWidth-1:0] d0;
input we0;
input reset;
input clk;

reg [DataWidth-1:0] ram[0:AddressRange-1];

initial begin
    $readmemh("./boom_core_top_boom_core_cycle_io_state_rob_entries_uop_exc_cause_RAM_AUTO_0R0W.dat", ram);
end



always @(posedge clk)  
begin 
    if (ce0) begin
        if (we0) 
            ram[address0] <= d0; 
    end
end


endmodule

