`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

module gps (
    input logic gps_clk, core_clk, nrst,
    input logic signal_in,
    input logic if_clk,
    inout wire [15:0] if_dat,
    input logic if_cs,
    output logic epoch_int
);

    logic [31:0] if_data_in;
    logic [31:0] if_data_out;
    logic [14:0] if_addr_out;
    logic if_read;
    logic if_write;

    parallel_interface if_dev (
        .clk(core_clk),
        .nrst(nrst),
        .data_in(if_data_in),
        .data_out(if_data_out),
        .addr_out(if_addr_out),
        .read(if_read),
        .write(if_write),
        .if_clk(if_clk),
        .if_cs(if_cs),
        .if_dat(if_dat)
    );

    logic snapshot;
    global_control_unit #(
        .REG_OFFSET(15'h00)
    ) global_control (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .if_data_in(if_data_in),
        .snapshot(snapshot)
    );

    logic [31:0] if_data_in_s0;
    logic search0_start_gps;
    logic [5:0] search0_ch;
    l1ca_search_unit #(
        .REG_OFFSET(15'h10)
    ) search_unit_0 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_s0),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start_gps(search0_start_gps),
        .search_ch(search0_ch)
    );

    logic [31:0] if_data_in_ch0;
    logic epoch_int_ch0;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h20)
    ) channel_unit_0 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_ch0),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start((search0_start_gps && search0_ch == 6'd0)),
        .snapshot(snapshot),
        .epoch_int(epoch_int_ch0)
    );

    assign if_data_in = if_data_in_s0 | if_data_in_ch0;
    assign epoch_int = epoch_int_ch0;
endmodule