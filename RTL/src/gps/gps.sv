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


    // Parallel MCU/FPGA interface
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

    // Global control and status registers
    logic snapshot;
    logic [31:0] if_data_in_global;
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
        .if_data_in(if_data_in_global),
        .snapshot(snapshot)
    );

    // L1CA FFT search and registers
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

    // L1CA channel 0 and registers
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

    // L1CA channel 1 and registers
    logic [31:0] if_data_in_ch1;
    logic epoch_int_ch1;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h30)
    ) channel_unit_1 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_ch1),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start((search0_start_gps && search0_ch == 6'd1)),
        .snapshot(snapshot),
        .epoch_int(epoch_int_ch1)
    );

    // L1CA channel 2 and registers
    logic [31:0] if_data_in_ch2;
    logic epoch_int_ch2;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h40)
    ) channel_unit_2 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_ch2),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start((search0_start_gps && search0_ch == 6'd2)),
        .snapshot(snapshot),
        .epoch_int(epoch_int_ch2)
    );

    // L1CA channel 3 and registers
    logic [31:0] if_data_in_ch3;
    logic epoch_int_ch3;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h50)
    ) channel_unit_3 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_ch3),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start((search0_start_gps && search0_ch == 6'd3)),
        .snapshot(snapshot),
        .epoch_int(epoch_int_ch3)
    );

    // L1CA channel 4 and registers
    logic [31:0] if_data_in_ch4;
    logic epoch_int_ch4;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h60)
    ) channel_unit_4 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_ch4),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start((search0_start_gps && search0_ch == 6'd4)),
        .snapshot(snapshot),
        .epoch_int(epoch_int_ch4)
    );

    // L1CA channel 5 and registers
    logic [31:0] if_data_in_ch5;
    logic epoch_int_ch5;
    l1ca_channel_unit #(
        .REG_OFFSET(15'h70)
    ) channel_unit_5 (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .if_data_in(if_data_in_ch5),
        .if_data_out(if_data_out),
        .if_addr_out(if_addr_out),
        .if_read(if_read),
        .if_write(if_write),
        .search_start((search0_start_gps && search0_ch == 6'd5)),
        .snapshot(snapshot),
        .epoch_int(epoch_int_ch5)
    );

    assign if_data_in = if_data_in_global | if_data_in_s0 | if_data_in_ch0 | if_data_in_ch1 | if_data_in_ch2 | if_data_in_ch3 | if_data_in_ch4 | if_data_in_ch5;
    assign epoch_int = epoch_int_ch0 | epoch_int_ch1 | epoch_int_ch2 | epoch_int_ch3 | epoch_int_ch4 | epoch_int_ch5;
endmodule