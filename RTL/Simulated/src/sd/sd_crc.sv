`timescale 1ns/1ns

module sd_crc (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic [3:0]      data_in,        // Data lines
    input  logic            clear,          // Clear CRC
    input  logic            enable,         // Enable CRC
    output logic [15:0]     crc_out [0:3]   // CRC outputs for each data line
);

logic [15:0] next_crc_reg [0:3];
logic next_in;

always_ff @(posedge clk) begin
    if (!nrst) begin
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