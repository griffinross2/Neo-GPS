`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

module l1ca_search_unit # (
    parameter logic [14:0] REG_OFFSET = 15'h10
) (
    input logic gps_clk, core_clk, nrst,
    input logic signal_in,
    output logic [31:0] if_data_in,
    input logic [31:0] if_data_out,
    input logic [14:0] if_addr_out,
    input logic if_read,
    input logic if_write,

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
        if_data_in = 32'h0;
        next_search_start = search_start;
        next_search_sv = search_sv;
        next_search_ch = search_ch;

        case (if_addr_out)
            // Search control register
            (REG_OFFSET): begin
                // [31] Busy
                // [30] Start
                // [11:6] Channel
                // [5:0] SV
                if_data_in = {search_busy, search_start, 18'd0, search_ch, search_sv};
                if (if_write) begin
                    next_search_start = if_data_out[30];
                    next_search_ch = if_data_out[11:6];
                    next_search_sv = if_data_out[5:0];
                end
            end

            // Search results - Accumulator
            (REG_OFFSET + 15'd1): begin
                if_data_in = search_acc_out;
            end

            // Search results - code index, start index, doppler index
            (REG_OFFSET + 15'd2): begin
                // [20:15] - Doppler index [5:0]
                // [14:10] - Start index [4:0]
                // [9:0] Code index [9:0]
                if_data_in = {11'b0, search_dop_index, search_start_index, search_code_index};
            end

            default: begin 
                if_data_in = 32'h0;
            end
        endcase

        if (&search_start_clk_ack) begin
            next_search_start = 1'b0;
        end
    end
endmodule