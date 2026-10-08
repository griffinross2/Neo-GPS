// Asymmetric Dual-Port Block RAM with Two Clocks

module dpram_asymm (clka,clkb,ena,enb,wea,addra,addrb,dia,dob);

parameter DATA_WIDTH_A = 4;
parameter DATA_WIDTH_B = 4;
parameter ADDR_WIDTH_A = 11;
parameter ADDR_WIDTH_B = 11;
parameter RAM_SIZE = 2048;

input clka,clkb,ena,enb,wea;
input [(ADDR_WIDTH_A-1):0] addra,addrb;
input [(DATA_WIDTH_A-1):0] dia;
output [(DATA_WIDTH_B-1):0] dob;
reg [(RAM_SIZE-1):0] ram;
reg [(DATA_WIDTH_B-1):0] dob;

always @(posedge clka)
begin
if (ena)
begin
if (wea)
ram[((addra+1)*DATA_WIDTH_A)-1:(addra*DATA_WIDTH_A)] <= dia;
end
end

always @(posedge clkb)
begin
if (enb)
begin
dob <= ram[((addrb+1)*DATA_WIDTH_B)-1:(addrb*DATA_WIDTH_B)];
end
end

endmodule
