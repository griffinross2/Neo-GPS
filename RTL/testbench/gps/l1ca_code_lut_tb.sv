`timescale 1ns/1ns

`include "common_types.vh"
import common_types_pkg::*;

module l1ca_code_lut_tb;

    logic clk, nrst;
    logic en, set;
    integer sv;
    logic code, epoch;
    gps_chip_t chip, chip_in;

    l1ca_code dut (
        .clk(clk),
        .nrst(nrst),
        .en(en),
        .set(set),
        .chip_in(chip_in),
        .sv(sv[5:0]),
        .code(code),
        .epoch(epoch),
        .chip(chip)
    );

    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    initial begin
        nrst = 0;
        en = 0;
        set = 0;
        sv = '0;
        chip_in = '0;

        #10 nrst = 1;

        for (sv = 0; sv < 32; sv++) begin
            // Set the SV
            @(negedge clk);
            set = 1;
            @(negedge clk);
            set = 0;

            en = 1;
            wait(epoch == 1'b0);
            wait(epoch == 1'b1);
        end

        #10 $finish; // End simulation
    end

endmodule