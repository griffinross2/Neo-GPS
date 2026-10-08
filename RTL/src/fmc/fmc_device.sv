`timescale 1ns/1ns

// Peripheral to act as satellite for the STM32 Flexible Memory Controller (FMC) in muxed PSRAM mode.

module fmc_device #(
    parameter ADDR_WIDTH = 16,
    parameter EXT_WIDTH = 16,
    parameter INT_WIDTH = 32
) (
    input  logic                    clk,        // Clock signal
    input  logic                    nrst,       // Active low reset
    input  logic [INT_WIDTH-1:0]    data_in,    // Data to transmit
    output logic [INT_WIDTH-1:0]    data_out,   // Data received
    output logic [ADDR_WIDTH-1:0]   addr_out,   // Address output (to select data)
    output logic                    read,       // Read register data
    output logic                    write,      // Read register data
    input  logic                    fmc_clk,    // FMC clock
    input  logic                    fmc_ne,     // FMC enable
    input  logic                    fmc_nwe,    // FMC write enable
    input  logic                    fmc_noe,    // FMC output enable
    output logic                    fmc_nwait,  // FMC wait signal
    input  logic [EXT_WIDTH-1:0]    fmc_dat     // FMC address/data bus
);

    

endmodule