// Simple Dual-Port Block RAM with Two Clocks
// File: simple_dual_two_clocks.v
// Pasted and modified from AMD Xilinx UG901 (v2025.2)

module simple_dual_two_clocks (clka,clkb,ena,enb,wea,addra,addrb,dia,dob);

parameter DATA_WIDTH = 4;
parameter ADDR_WIDTH = 11;
parameter RAM_DEPTH = 2048;

input clka,clkb,ena,enb,wea;
input [(ADDR_WIDTH-1):0] addra,addrb;
input [(DATA_WIDTH-1):0] dia;
output [(DATA_WIDTH-1):0] dob;
reg [(DATA_WIDTH-1):0] ram [(RAM_DEPTH-1):0];
reg [(DATA_WIDTH-1):0] dob;

always @(posedge clka)
begin
if (ena)
begin
if (wea)
ram[addra] <= dia;
end
end

always @(posedge clkb)
begin
if (enb)
begin
dob <= ram[addrb];
end
end

endmodule
