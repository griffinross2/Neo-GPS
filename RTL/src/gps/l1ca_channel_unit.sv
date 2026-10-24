`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

module l1ca_channel_unit # (
    parameter logic [14:0] REG_OFFSET = 15'h20
) (
    input logic gps_clk, core_clk, nrst,
    input logic signal_in,
    output logic [31:0] if_data_in,
    input logic [31:0] if_data_out,
    input logic [14:0] if_addr_out,
    input logic if_read,
    input logic if_write,
    input logic search_start,
    input logic snapshot,
    output logic epoch_int
);

    sv_t ch_sv, next_ch_sv; // SV number for channel
    logic ch_epoch; // Epoch signal from channel
    logic [3:0] ch_epoch_sync_clk; // Sync the epoch signal to the core clock domain
    logic ch_epoch_reg_gps; // Register for the epoch signal in the gps clock domain
    logic ch_epoch_reg_clk, next_ch_epoch_reg_clk; // Register for the epoch signal in the core clock domain
    logic ch_clear, next_ch_clear; // Clear channel
    logic ch_epoch_counter_clear_clk, next_ch_epoch_counter_clear_clk; // Counter for the epoch signal in the core clock domain
    logic [1:0] ch_epoch_counter_clear_sync_gps; // Sync the epoch counter clear signal to the GPS clock domain
    logic [1:0] ch_epoch_counter_clear_ack_clk; // Ack for the above
    logic [31:0] ch_code_rate_gps, next_ch_code_rate_gps, ch_lo_rate_gps, next_ch_lo_rate_gps; // Code and local oscillator rates in GPS clock domain
    logic [31:0] ch_code_rate_clk, next_ch_code_rate_clk, ch_lo_rate_clk, next_ch_lo_rate_clk; // Code and local oscillator rates in core clock domain
    logic ch_code_rate_done_clk, next_ch_code_rate_done_clk, ch_lo_rate_done_clk, next_ch_lo_rate_done_clk; // Done flags after code and lo rate are written
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
    logic [12:0] ch_epoch_counter, next_ch_epoch_counter;

    logic [31:0] ch_code_snapshot, ch_code_snapshot_clk;
    logic [31:0] ch_lo_snapshot, ch_lo_snapshot_clk;
    gps_chip_t ch_chip_snapshot, ch_chip_snapshot_clk;
    logic [12:0] ch_epoch_counter_snapshot, ch_epoch_counter_snapshot_clk;
    
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

    assign epoch_int = ch_epoch_reg_clk;

    always_ff @(posedge core_clk or negedge nrst) begin
        if (!nrst) begin
            ch_code_rate_clk <= 32'd228841226;
            ch_lo_rate_clk <= 32'd899258778;
            ch_clear <= '0;
            ch_sv <= '0;
            ch_epoch_sync_clk <= '0;
            ch_epoch_reg_clk <= '0;
            ch_pause_clk <= '0;
            ch_pause_delay <= '0;
            ch_pause_ack_clk <= '0;
            ch_code_rate_done_clk <= '0;
            ch_lo_rate_done_clk <= '0;
            ch_code_rate_ack_clk <= '0;
            ch_lo_rate_ack_clk <= '0;
            ch_epoch_counter_clear_clk <= '0;
            ch_epoch_counter_clear_ack_clk <= '0;
        end else begin
            ch_code_rate_clk <= next_ch_code_rate_clk;
            ch_lo_rate_clk <= next_ch_lo_rate_clk;
            ch_clear <= next_ch_clear;
            ch_sv <= next_ch_sv;
            ch_epoch_sync_clk <= {ch_epoch_sync_clk[2:0], ch_epoch_reg_gps};
            ch_epoch_reg_clk <= next_ch_epoch_reg_clk;
            ch_pause_clk <= next_ch_pause_clk;
            ch_pause_delay <= next_ch_pause_delay;
            ch_pause_ack_clk <= {ch_pause_ack_clk[0], &ch_pause_sync_gps};
            ch_code_rate_done_clk <= next_ch_code_rate_done_clk;
            ch_lo_rate_done_clk <= next_ch_lo_rate_done_clk;
            ch_code_rate_ack_clk <= {ch_code_rate_ack_clk[0], &ch_code_rate_sync_gps};
            ch_lo_rate_ack_clk <= {ch_lo_rate_ack_clk[0], &ch_lo_rate_sync_gps};
            ch_epoch_counter_clear_clk <= next_ch_epoch_counter_clear_clk;
            ch_epoch_counter_clear_ack_clk <= {ch_epoch_counter_clear_ack_clk[0], &ch_epoch_counter_clear_sync_gps};
        end
    end

    always_ff @(posedge gps_clk or negedge nrst) begin
        if (!nrst) begin
            ch_pause_sync_gps <= '0;
            ch_code_rate_sync_gps <= '0;
            ch_lo_rate_sync_gps <= '0;
            ch_code_rate_gps <= 32'd228841226;
            ch_lo_rate_gps <= 32'd899258778;
            ch_epoch_counter_clear_sync_gps <= '0;
            ch_epoch_counter <= '0;
            ch_ie_reg <= '0;
            ch_qe_reg <= '0;
            ch_ip_reg <= '0;
            ch_qp_reg <= '0;
            ch_il_reg <= '0;
            ch_ql_reg <= '0;
            ch_code_snapshot <= '0;
            ch_lo_snapshot <= '0;
            ch_chip_snapshot <= '0;
            ch_epoch_counter_snapshot <= '0;
            ch_epoch_reg_gps <= '0;
        end else begin
            ch_pause_sync_gps <= {ch_pause_sync_gps[2:0], ch_pause_clk};
            ch_code_rate_sync_gps <= {ch_code_rate_sync_gps[0], ch_code_rate_done_clk};
            ch_lo_rate_sync_gps <= {ch_lo_rate_sync_gps[0], ch_lo_rate_done_clk};
            ch_code_rate_gps <= next_ch_code_rate_gps;
            ch_lo_rate_gps <= next_ch_lo_rate_gps;
            ch_epoch_counter_clear_sync_gps <= {ch_epoch_counter_clear_sync_gps[0], ch_epoch_counter_clear_clk};
            ch_epoch_counter <= next_ch_epoch_counter;
            ch_ie_reg <= ch_epoch ? ch_ie : ch_ie_reg;
            ch_qe_reg <= ch_epoch ? ch_qe : ch_qe_reg;
            ch_ip_reg <= ch_epoch ? ch_ip : ch_ip_reg;
            ch_qp_reg <= ch_epoch ? ch_qp : ch_qp_reg;
            ch_il_reg <= ch_epoch ? ch_il : ch_il_reg;
            ch_ql_reg <= ch_epoch ? ch_ql : ch_ql_reg;
            ch_code_snapshot <= snapshot ? ch_code_phase : ch_code_snapshot;
            ch_lo_snapshot <= snapshot ? ch_lo_phase : ch_lo_snapshot;
            ch_chip_snapshot <= snapshot ? ch_chip : ch_chip_snapshot;
            ch_epoch_counter_snapshot <= snapshot ? ch_epoch_counter : ch_epoch_counter_snapshot;
            ch_epoch_reg_gps <= ch_epoch;
        end
    end

    always_comb begin
        if_data_in = 32'h0;
        next_ch_clear = ch_clear;
        next_ch_sv = ch_sv;
        next_ch_epoch_reg_clk = ch_epoch_reg_clk;
        next_ch_pause_clk = ch_pause_clk;
        next_ch_pause_delay = ch_pause_delay;
        next_ch_code_rate_clk = ch_code_rate_clk;
        next_ch_lo_rate_clk = ch_lo_rate_clk;
        next_ch_code_rate_done_clk = ch_code_rate_done_clk;
        next_ch_lo_rate_done_clk = ch_lo_rate_done_clk;
        next_ch_code_rate_gps = ch_code_rate_gps;
        next_ch_lo_rate_gps = ch_lo_rate_gps;
        next_ch_epoch_counter = ch_epoch_counter;
        next_ch_epoch_counter_clear_clk = ch_epoch_counter_clear_clk;

        case (if_addr_out)
            // channel control register
            (REG_OFFSET): begin
                if_data_in = {ch_epoch_reg_clk, ch_clear, 11'd0, ch_epoch_counter, ch_sv};
                if (if_write) begin
                    next_ch_clear = if_data_out[30];
                    next_ch_epoch_counter_clear_clk = if_data_out[29] ? 1'b1 : ch_epoch_counter_clear_clk;
                    next_ch_sv = if_data_out[5:0];
                end
                if (if_read) begin
                    // Clear epoch signal on read
                    // This will be overriden if an epoch is occurring on the same
                    // clock cycle. Therefore, we perform the check for that after
                    // so that we don't miss an epoch.
                    next_ch_epoch_reg_clk = 1'b0;
                end
            end

            // channel pause register
            (REG_OFFSET + 15'h1): begin
                if_data_in = {16'd0, ch_pause_delay};
                if (if_write) begin
                    next_ch_pause_delay = if_data_out[15:0];
                    next_ch_pause_clk = 1'b1;
                end
            end

            // channel code rate
            (REG_OFFSET + 15'h2): begin
                if_data_in = '0;
                if (if_write) begin
                    next_ch_code_rate_clk = if_data_out;
                    next_ch_code_rate_done_clk = 1'b1;
                end
            end

            // channel lo rate
            (REG_OFFSET + 15'h3): begin
                if_data_in = '0;
                if (if_write) begin
                    next_ch_lo_rate_clk = if_data_out;
                    next_ch_lo_rate_done_clk = 1'b1;
                end
            end

            // epoch counter, chip snapshot
            (REG_OFFSET + 15'h4): begin
                // 28-16 = epoch counter, 9-0 = chip
                if_data_in = {3'd0, ch_epoch_counter_snapshot, 6'd0, ch_chip_snapshot};
            end

            // code snapshot
            (REG_OFFSET + 15'h5): begin
                if_data_in = ch_code_snapshot;
            end

            // lo snapshot
            (REG_OFFSET + 15'h6): begin
                if_data_in = ch_lo_snapshot;
            end

            // channel accumulators
            (REG_OFFSET + 15'h7): begin
                // IP
                if_data_in = ch_ip_reg;
            end

            (REG_OFFSET + 15'h8): begin
                // QP
                if_data_in = ch_qp_reg;
            end

            (REG_OFFSET + 15'h9): begin
                // IE
                if_data_in = ch_ie_reg;
            end

            (REG_OFFSET + 15'hA): begin
                // QE
                if_data_in = ch_qe_reg;
            end

            (REG_OFFSET + 15'hB): begin
                // IL
                if_data_in = ch_il_reg;
            end

            (REG_OFFSET + 15'hC): begin
                // QL
                if_data_in = ch_ql_reg;
            end

            default: begin 
                if_data_in = 32'h0;
            end
        endcase

        if (ch_epoch_sync_clk[3:2] == 2'b01) begin
            // Set epoch register high on epoch pulse rising edge
            next_ch_epoch_reg_clk = 1'b1;
        end

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

        if (ch_epoch) begin
            next_ch_epoch_counter = (ch_epoch_counter + 13'd1) % 13'd6000;
        end

        if (&ch_epoch_counter_clear_sync_gps) begin
            next_ch_epoch_counter = 13'd1;
        end

        if (&ch_epoch_counter_clear_ack_clk) begin
            next_ch_epoch_counter_clear_clk = 1'b0;
        end
    end
endmodule