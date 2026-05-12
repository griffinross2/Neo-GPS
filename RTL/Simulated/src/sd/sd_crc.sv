`timescale 1ns/1ns

module sd_crc16 (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic [3:0]      data_in,        // Data lines
    input  logic            clear,          // Clear CRC
    input  logic            enable,         // Enable CRC
    output logic [15:0]     crc_out [0:3]   // CRC outputs for each data line
);

logic [15:0] next_crc_reg [0:3];
logic next_in;

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        crc_out[0] <= '1;
        crc_out[1] <= '1;
        crc_out[2] <= '1;
        crc_out[3] <= '1;
    end else begin
        crc_out[0] <= next_crc_reg[0];
        crc_out[1] <= next_crc_reg[1];
        crc_out[2] <= next_crc_reg[2];
        crc_out[3] <= next_crc_reg[3];
    end
end

always_comb begin
    next_in = 1'b0;
    for (int i = 0; i < 4; i++) begin
        if (clear) begin
            next_crc_reg[i] = '1;
        end else if (enable) begin
            next_in = data_in[i] ^ crc_out[i][15];
            next_crc_reg[i] = {crc_out[i][14:0], 1'b0} ^ (next_in ? 16'h1021 : 16'h0000);
        end else begin
            next_crc_reg[i] = crc_out[i];
        end
    end
end

endmodule

module sd_crc7 (
    input   logic           clk,            // Clock signal
    input   logic           nrst,           // Active low reset
    input   logic           cmd_in,         // CMD to calculate CRC over
    input   logic           clear,          // Clear CRC
    input   logic           enable,         // Enable CRC calculation
    output  logic [6:0]     crc_out         // CRC output
);
    
logic [6:0] next_crc_reg;
logic next_in;

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        crc_out <= '0;
    end else begin
        crc_out <= next_crc_reg;
    end
end

always_comb begin
    next_in = 1'b0;

    if (clear) begin
        next_crc_reg = '0;
    end else if (enable) begin
        next_in = cmd_in ^ crc_out[6];
        next_crc_reg = {crc_out[5:0], 1'b0} ^ (next_in ? 7'h09 : 7'h00);
    end else begin
        next_crc_reg = crc_out;
    end
end

endmodule