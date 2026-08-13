`timescale 1ns/1ns

`include "common_types.vh"
import common_types_pkg::*;

module l1ca_code (
    input logic clk, nrst,
    input logic en, set,
    input gps_chip_t chip_in,
    input sv_t sv,
    output logic code, epoch,
    output gps_chip_t chip
);

    sv_t sv_reg;
    gps_chip_t next_chip;

    always_ff @(posedge clk or negedge nrst) begin
        if (!nrst) begin
            sv_reg <= '0;
            chip <= '0;
        end else begin
            sv_reg <= set ? sv : sv_reg;
            chip <= next_chip;
        end
    end

    always_comb begin
        next_chip = chip;
        epoch = (chip == '0);

        if (en) begin
            next_chip = (chip + 10'd1) % 10'd1023;
        end

        if (set) begin
            next_chip = chip_in;
        end
    end

    (* rom_style = "block" *) 
    reg code_rom [0:32735];

    initial begin
        $readmemh("l1ca_code_rom.hex", code_rom);
    end

    always_ff @(posedge clk) begin
        code <= code_rom[{sv_reg[4:0], next_chip}];
    end

endmodule