`timescale 1ns/1ns

module sd_crc16 (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic [3:0]      data_in,        // Data lines
    input  logic            clear,          // Clear CRC
    input  logic            enable,         // Enable CRC
    output logic [15:0]     crc_out0,       // CRC output for DAT0
    output logic [15:0]     crc_out1,       // CRC output for DAT1
    output logic [15:0]     crc_out2,       // CRC output for DAT2
    output logic [15:0]     crc_out3        // CRC output for DAT3
);

logic [15:0] next_crc_reg0;
logic [15:0] next_crc_reg1;
logic [15:0] next_crc_reg2;
logic [15:0] next_crc_reg3;
logic next_in0;
logic next_in1;
logic next_in2;
logic next_in3;

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        crc_out0 <= '0;
        crc_out1 <= '0;
        crc_out2 <= '0;
        crc_out3 <= '0;
    end else begin
        crc_out0 <= next_crc_reg0;
        crc_out1 <= next_crc_reg1;
        crc_out2 <= next_crc_reg2;
        crc_out3 <= next_crc_reg3;
    end
end

always_comb begin
    next_in0 = 1'b0;
    next_in1 = 1'b0;
    next_in2 = 1'b0;
    next_in3 = 1'b0;
    if (clear) begin
        next_crc_reg0 = '0;
        next_crc_reg1 = '0;
        next_crc_reg2 = '0;
        next_crc_reg3 = '0;
    end else if (enable) begin
        next_in0 = data_in[0] ^ crc_out0[15];
        next_in1 = data_in[1] ^ crc_out1[15];
        next_in2 = data_in[2] ^ crc_out2[15];
        next_in3 = data_in[3] ^ crc_out3[15];
        next_crc_reg0 = {crc_out0[14:0], 1'b0} ^ (next_in0 ? 16'h1021 : 16'h0000);
        next_crc_reg1 = {crc_out1[14:0], 1'b0} ^ (next_in1 ? 16'h1021 : 16'h0000);
        next_crc_reg2 = {crc_out2[14:0], 1'b0} ^ (next_in2 ? 16'h1021 : 16'h0000);
        next_crc_reg3 = {crc_out3[14:0], 1'b0} ^ (next_in3 ? 16'h1021 : 16'h0000);
    end else begin
        next_crc_reg0 = crc_out0;
        next_crc_reg1 = crc_out1;
        next_crc_reg2 = crc_out2;
        next_crc_reg3 = crc_out3;
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