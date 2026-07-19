`timescale 1ns/1ns

module memory_unit (
    input logic clk, nrst,

    input logic mem_we,
    input logic [11:0] mem_addr,
    input logic signed [15:0] mem_wdata_re, mem_wdata_im,
    output logic signed [15:0] mem_rdata_re, mem_rdata_im
);

spram #(
    .DATA_WIDTH(32),
    .ADDR_WIDTH(12),
    .RAM_DEPTH(4096)
) mem_inst (
    .clka(clk),
    .ena(1'b1),
    .wea(mem_we),
    .addra(mem_addr),
    .dia({mem_wdata_re, mem_wdata_im}),
    .doa({mem_rdata_re, mem_rdata_im})
);

endmodule