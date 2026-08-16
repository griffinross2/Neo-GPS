`timescale 1ns/1ns

`include "common_types.vh"
import common_types_pkg::*;

module l1ca_search_unit # (
    parameter logic [14:0] REG_OFFSET = 15'h10
) (
    input logic gps_clk, core_clk, nrst,
    input logic signal_in,
    output logic [15:0] spi_data_in,
    input logic [15:0] spi_data_out,
    input logic [14:0] spi_addr_out,
    input logic spi_read,
    input logic spi_write,

    output logic search_start_gps,
    output logic [5:0] search_ch
);

    assign search_start_gps = &search_start_gps_sync;

    logic search_start;                // Start acquisition
    sv_t search_sv;                    // SV number to search
    word_t search_acc_out;             // Maximum correlation
    logic [9:0] search_code_index;     // Chip index of maximum correlation -> 0 thru 1022
    logic [4:0] search_start_index;    // Sample start index of maximum correlation -> 0 thru 18
    logic [5:0] search_dop_index;      // Doppler index of maximum correlation 0 thru 40 -> -5000 thru 5000 Hz in 250 Hz steps
    logic search_busy;

    l1ca_ac_pca_search search_unit (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .start(search_start),
        .signal_in(signal_in),
        .sv(search_sv),
        .acc_out(search_acc_out),
        .code_index(search_code_index),
        .start_index(search_start_index),
        .dop_index(search_dop_index),
        .busy(search_busy)
    );

    logic next_search_start;
    logic [1:0] search_start_gps_sync;
    logic [1:0] search_start_clk_ack;
    sv_t next_search_sv;
    logic [5:0] next_search_ch;

    always_ff @(posedge core_clk or negedge nrst) begin
        if (!nrst) begin
            search_start <= '0;
            search_sv <= '0;
            search_ch <= '0;
            search_start_clk_ack <= '0;
        end else begin
            search_start <= next_search_start;
            search_sv <= next_search_sv;
            search_ch <= next_search_ch;
            search_start_clk_ack <= {search_start_clk_ack[0], &search_start_gps_sync};
        end
    end

    always_ff @(posedge gps_clk or negedge nrst) begin
        if (!nrst) begin
            search_start_gps_sync <= '0;
        end else begin
            search_start_gps_sync <= {search_start_gps_sync[0], search_start};
        end
    end

    always_comb begin
        spi_data_in = 16'h0000;
        next_search_start = search_start;
        next_search_sv = search_sv;
        next_search_ch = search_ch;

        case (spi_addr_out)
            // Search 0 control register
            (REG_OFFSET): begin
                // [15] Busy
                // [14] Start
                // [11:6] Channel
                // [5:0] SV
                spi_data_in = {search_busy, search_start, 2'd0, search_ch, search_sv};
                if (spi_write) begin
                    next_search_start = spi_data_out[14];
                    next_search_sv = spi_data_out[5:0];
                    next_search_ch = spi_data_out[11:6];
                end
            end

            // Search 0 results - Accumulator
            (REG_OFFSET + 15'd1): begin
                spi_data_in = search_acc_out[15:0];
            end
            (REG_OFFSET + 15'd2): begin
                spi_data_in = search_acc_out[31:16];
            end

            // Search 0 results - code index, start index, doppler index
            (REG_OFFSET + 15'd3): begin
                // [15:11] - Start index [4:0]
                // [9:0] Code index [9:0]
                spi_data_in = {search_start_index, 1'b0, search_code_index};
            end
            (REG_OFFSET + 15'd4): begin
                // [7:6] - reserved
                // [5:0] - doppler index [5:0]
                spi_data_in = {10'b0, search_dop_index[5:0]};
            end

            default: begin 
                spi_data_in = 16'h0000;
            end
        endcase

        if (&search_start_clk_ack) begin
            next_search_start = 1'b0;
        end
    end
endmodule