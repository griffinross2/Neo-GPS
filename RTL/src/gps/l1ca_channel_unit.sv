`timescale 1ns/1ns

`include "common_types.vh"
import common_types_pkg::*;

module l1ca_channel_unit # (
    parameter logic [14:0] REG_OFFSET = 15'h20
) (
    input logic gps_clk, core_clk, nrst,
    input logic signal_in,
    output logic [15:0] spi_data_in,
    input logic [15:0] spi_data_out,
    input logic [14:0] spi_addr_out,
    input logic spi_read,
    input logic spi_write,

    input logic search_start
);

    sv_t ch_sv, next_ch_sv; // SV number for channel
    logic ch_epoch; // Epoch signal from channel
    logic ch_clear, next_ch_clear; // Clear channel
    logic [31:0] ch_code_rate_gps, next_ch_code_rate_gps, ch_lo_rate_gps, next_ch_lo_rate_gps; // Code and local oscillator rates in GPS clock domain
    logic [31:0] ch_code_rate_clk, next_ch_code_rate_clk, ch_lo_rate_clk, next_ch_lo_rate_clk; // Code and local oscillator rates in core clock domain
    logic ch_code_rate_done_clk, next_ch_code_rate_done_clk, ch_lo_rate_done_clk, next_ch_lo_rate_done_clk; // Done flags after HIGH reg of code and lo rate are written
    logic [1:0] ch_code_rate_sync_gps, ch_lo_rate_sync_gps; // Sync the done flag to load the code and lo rate all at once in the GPS clock domain
    logic [1:0] ch_code_rate_ack_clk, ch_lo_rate_ack_clk; // Acknowledge the done flag in the core clock domain
    logic [31:0] ch_code_phase, ch_lo_phase;
    logic [15:0] ch_pause_delay, next_ch_pause_delay;
    logic ch_pause_clk, next_ch_pause_clk;
    logic [3:0] ch_pause_sync_gps;
    logic [1:0] ch_pause_ack_clk;
    gps_chip_t ch_chip;
    acc_t ch_ie, ch_qe, ch_ip, ch_qp, ch_il, ch_ql;
    acc_t ch_ie_reg, ch_qe_reg, ch_ip_reg, ch_qp_reg, ch_il_reg, ch_ql_reg;
    
    l1ca_channel ch (
        .clk(gps_clk),
        .nrst(nrst),
        .start(search_start),
        .clear(ch_clear),
        .sv(ch_sv),
        .code_rate(ch_code_rate_gps),
        .lo_rate(ch_lo_rate_gps),
        .signal_in(signal_in),
        .pause((|ch_pause_sync_gps[3:2]) & (~|ch_pause_sync_gps[1:0])),
        .pause_delay(ch_pause_delay),
        .epoch(ch_epoch),
        .chip(ch_chip),
        .ie(ch_ie),
        .qe(ch_qe),
        .ip(ch_ip),
        .qp(ch_qp),
        .il(ch_il),
        .ql(ch_ql),
        .code_phase(ch_code_phase),
        .lo_phase(ch_lo_phase)
    );

    always_ff @(posedge core_clk or negedge nrst) begin
        if (!nrst) begin
            ch_code_rate_clk <= 32'd228841226;
            ch_lo_rate_clk <= 32'd899258778;
            ch_clear <= '0;
            ch_sv <= '0;
            ch_pause_clk <= '0;
            ch_pause_delay <= '0;
            ch_pause_ack_clk <= '0;
            ch_ie_reg <= '0;
            ch_qe_reg <= '0;
            ch_ip_reg <= '0;
            ch_qp_reg <= '0;
            ch_il_reg <= '0;
            ch_ql_reg <= '0;
            ch_code_rate_done_clk <= '0;
            ch_lo_rate_done_clk <= '0;
            ch_code_rate_ack_clk <= '0;
            ch_lo_rate_ack_clk <= '0;
        end else begin
            ch_code_rate_clk <= next_ch_code_rate_clk;
            ch_lo_rate_clk <= next_ch_lo_rate_clk;
            ch_clear <= next_ch_clear;
            ch_sv <= next_ch_sv;
            ch_pause_clk <= next_ch_pause_clk;
            ch_pause_delay <= next_ch_pause_delay;
            ch_pause_ack_clk <= {ch_pause_ack_clk[0], &ch_pause_sync_gps};
            ch_ie_reg <= ch_epoch ? ch_ie : ch_ie_reg;
            ch_qe_reg <= ch_epoch ? ch_qe : ch_qe_reg;
            ch_ip_reg <= ch_epoch ? ch_ip : ch_ip_reg;
            ch_qp_reg <= ch_epoch ? ch_qp : ch_qp_reg;
            ch_il_reg <= ch_epoch ? ch_il : ch_il_reg;
            ch_ql_reg <= ch_epoch ? ch_ql : ch_ql_reg;
            ch_code_rate_done_clk <= next_ch_code_rate_done_clk;
            ch_lo_rate_done_clk <= next_ch_lo_rate_done_clk;
            ch_code_rate_ack_clk <= {ch_code_rate_ack_clk[0], &ch_code_rate_sync_gps};
            ch_lo_rate_ack_clk <= {ch_lo_rate_ack_clk[0], &ch_lo_rate_sync_gps};
        end
    end

    always_ff @(posedge gps_clk or negedge nrst) begin
        if (!nrst) begin
            ch_pause_sync_gps <= '0;
            ch_code_rate_sync_gps <= '0;
            ch_lo_rate_sync_gps <= '0;
            ch_code_rate_gps <= 32'd228841226;
            ch_lo_rate_gps <= 32'd899258778;
        end else begin
            ch_pause_sync_gps <= {ch_pause_sync_gps[2:0], ch_pause_clk};
            ch_code_rate_sync_gps <= {ch_code_rate_sync_gps[0], ch_code_rate_done_clk};
            ch_lo_rate_sync_gps <= {ch_lo_rate_sync_gps[0], ch_lo_rate_done_clk};
            ch_code_rate_gps <= next_ch_code_rate_gps;
            ch_lo_rate_gps <= next_ch_lo_rate_gps;
        end
    end

    always_comb begin
        spi_data_in = 16'h0000;
        next_ch_clear = ch_clear;
        next_ch_sv = ch_sv;
        next_ch_pause_clk = ch_pause_clk;
        next_ch_pause_delay = ch_pause_delay;
        next_ch_code_rate_clk = ch_code_rate_clk;
        next_ch_lo_rate_clk = ch_lo_rate_clk;
        next_ch_code_rate_done_clk = ch_code_rate_done_clk;
        next_ch_lo_rate_done_clk = ch_lo_rate_done_clk;
        next_ch_code_rate_gps = ch_code_rate_gps;
        next_ch_lo_rate_gps = ch_lo_rate_gps;

        case (spi_addr_out)
            // channel control register
            (REG_OFFSET): begin
                spi_data_in = {ch_clear, ch_epoch, 8'b0, ch_sv};
                if (spi_write) begin
                    next_ch_clear = spi_data_out[15];
                    next_ch_sv = spi_data_out[5:0];
                end
            end

            // channel pause register
            (REG_OFFSET + 15'h1): begin
                spi_data_in = ch_pause_delay;
                if (spi_write) begin
                    next_ch_pause_delay = spi_data_out;
                    next_ch_pause_clk = 1'b1;
                end
            end

            // channel code rate low
            (REG_OFFSET + 15'h2): begin
                spi_data_in = '0;
                if (spi_write) begin
                    next_ch_code_rate_clk[15:0] = spi_data_out;
                end
            end

            // channel code rate high
            (REG_OFFSET + 15'h3): begin
                spi_data_in = '0;
                if (spi_write) begin
                    next_ch_code_rate_clk[31:16] = spi_data_out;
                    next_ch_code_rate_done_clk = 1'b1;
                end
            end

            // channel lo rate low
            (REG_OFFSET + 15'h4): begin
                spi_data_in = '0;
                if (spi_write) begin
                    next_ch_lo_rate_clk[15:0] = spi_data_out;
                end
            end

            // channel lo rate high
            (REG_OFFSET + 15'h5): begin
                spi_data_in = '0;
                if (spi_write) begin
                    next_ch_lo_rate_clk[31:16] = spi_data_out;
                    next_ch_lo_rate_done_clk = 1'b1;
                end
            end

            // channel accumulators
            (REG_OFFSET + 15'h10): begin
                // IP low
                spi_data_in = ch_ip_reg[15:0];
            end

            (REG_OFFSET + 15'h11): begin
                // IP high
                spi_data_in = ch_ip_reg[31:16];
            end

            (REG_OFFSET + 15'h12): begin
                // QP low
                spi_data_in = ch_qp_reg[15:0];
            end

            (REG_OFFSET + 15'h13): begin
                // QP high
                spi_data_in = ch_qp_reg[31:16];
            end

            (REG_OFFSET + 15'h14): begin
                // IE low
                spi_data_in = ch_ie_reg[15:0];
            end

            (REG_OFFSET + 15'h15): begin
                // IE high
                spi_data_in = ch_ie_reg[31:16];
            end

            (REG_OFFSET + 15'h16): begin
                // QE low
                spi_data_in = ch_qe_reg[15:0];
            end

            (REG_OFFSET + 15'h17): begin
                // QE high
                spi_data_in = ch_qe_reg[31:16];
            end

            (REG_OFFSET + 15'h18): begin
                // IL low
                spi_data_in = ch_il_reg[15:0];
            end

            (REG_OFFSET + 15'h19): begin
                // IL high
                spi_data_in = ch_il_reg[31:16];
            end

            (REG_OFFSET + 15'h1A): begin
                // QL low
                spi_data_in = ch_ql_reg[15:0];
            end

            (REG_OFFSET + 15'h1B): begin
                // QL high
                spi_data_in = ch_ql_reg[31:16];
            end

            default: begin 
                spi_data_in = 16'h0000;
            end
        endcase

        if (&ch_pause_ack_clk) begin
            next_ch_pause_clk = 1'b0;
        end

        if (&ch_code_rate_ack_clk) begin
            next_ch_code_rate_done_clk = 1'b0;
        end

        if (&ch_lo_rate_ack_clk) begin
            next_ch_lo_rate_done_clk = 1'b0;
        end

        if (&ch_code_rate_sync_gps) begin
            next_ch_code_rate_gps = ch_code_rate_clk;
        end

        if (&ch_lo_rate_sync_gps) begin
            next_ch_lo_rate_gps = ch_lo_rate_clk;
        end
    end
endmodule