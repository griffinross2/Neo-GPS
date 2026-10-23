`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

// Contains global control registers

module global_control_unit # (
    parameter logic [14:0] REG_OFFSET = 15'h00
) (
    input logic gps_clk, core_clk, nrst,
    input logic [31:0] if_data_out,
    input logic [14:0] if_addr_out,
    input logic if_read,
    input logic if_write,
    output logic [31:0] if_data_in,

    output logic snapshot
);

logic snapshot_clk, next_snapshot_clk;
logic [2:0] snapshot_sync_gps;
logic [1:0] snapshot_ack_clk;

always_ff @(posedge gps_clk or negedge nrst) begin
    if (!nrst) begin
        snapshot_sync_gps <= 3'b0;
    end else begin
        snapshot_sync_gps <= {snapshot_sync_gps[1:0], snapshot_clk};
    end
end

always_ff @(posedge core_clk or negedge nrst) begin
    if (!nrst) begin
        snapshot_clk <= 1'b0;
        snapshot_ack_clk <= 2'b0;
    end else begin
        snapshot_clk <= next_snapshot_clk;
        snapshot_ack_clk <= {snapshot_ack_clk[0], &snapshot_sync_gps[1:0]};
    end
end

assign snapshot = (snapshot_sync_gps == 3'b011) ? 1'b1 : 1'b0;

always_comb begin
    next_snapshot_clk = snapshot_clk;
    if_data_in = 32'b0;

    case (if_addr_out)
        REG_OFFSET + 15'h01: begin
            if (if_write) begin
                // Set snapshot request
                next_snapshot_clk = if_data_out[0] ? 1'b1 : snapshot_clk;
            end
        end

        default: begin
            if_data_in = 32'b0;
        end
    endcase

    if (&snapshot_ack_clk) begin
        next_snapshot_clk = 1'b0;
    end
end

endmodule