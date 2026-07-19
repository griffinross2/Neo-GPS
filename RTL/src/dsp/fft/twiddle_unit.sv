`timescale 1ns/1ns

module twiddle_unit (
    input logic clk,

    input logic [10:0] tw_idx,
    output logic signed [15:0] tw_re, tw_im
);

reg [31:0] twiddle_data;

(* rom_style = "block" *) 
reg [31:0] twiddle_rom [0:2047];

initial begin
    $readmemh("twiddle_rom.hex", twiddle_rom);
end

always_ff @(posedge clk) begin
    twiddle_data <= twiddle_rom[tw_idx];
end

assign tw_re = twiddle_data[31:16];
assign tw_im = twiddle_data[15:0];

endmodule