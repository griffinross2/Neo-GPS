`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

module gps (
    input logic gps_clk, core_clk, nrst,
    input logic signal_in,
    input logic sck,
    input logic sdi,
    output logic sdo,
    input logic cs
);

    logic [15:0] spi_data_in;
    logic [15:0] spi_data_out;
    logic [14:0] spi_addr_out;
    logic spi_read;
    logic spi_write;

    spi_device spi_dev (
        .clk(core_clk),
        .nrst(nrst),
        .data_in(spi_data_in),
        .data_out(spi_data_out),
        .addr_out(spi_addr_out),
        .read(spi_read),
        .write(spi_write),
        .sck(sck),
        .sdi(sdi),
        .sdo(sdo),
        .cs(cs)
    );

    logic [15:0] spi_data_in_s0;
    logic search0_start_gps;
    logic [5:0] search0_ch;
    l1ca_search_unit #(
        .REG_OFFSET(15'h10)
    ) search_unit_0 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .spi_data_in(spi_data_in_s0),
        .spi_data_out(spi_data_out),
        .spi_addr_out(spi_addr_out),
        .spi_read(spi_read),
        .spi_write(spi_write),
        .search_start_gps(search0_start_gps),
        .search_ch(search0_ch)
    );

    logic [15:0] spi_data_in_ch0;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h20)
    ) channel_unit_0 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .spi_data_in(spi_data_in_ch0),
        .spi_data_out(spi_data_out),
        .spi_addr_out(spi_addr_out),
        .spi_read(spi_read),
        .spi_write(spi_write),
        .search_start((search0_start_gps && search0_ch == 6'd0))
    );

    assign spi_data_in = spi_data_in_s0 | spi_data_in_ch0;
endmodule