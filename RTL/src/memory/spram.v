module spram (clka,ena,wea,addra,dia,doa);

parameter DATA_WIDTH = 4;
parameter ADDR_WIDTH = 11;
parameter RAM_DEPTH = 2048;

input clka,ena,wea;
input [(ADDR_WIDTH-1):0] addra;
input [(DATA_WIDTH-1):0] dia;
output [(DATA_WIDTH-1):0] doa;
reg [(DATA_WIDTH-1):0] ram [(RAM_DEPTH-1):0];
reg [(DATA_WIDTH-1):0] doa;

always @(posedge clka)
begin
if (ena)
begin
if (wea)
ram[addra] <= dia;
end
doa <= ram[addra];
end

endmodule
