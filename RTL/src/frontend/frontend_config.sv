`timescale 1ns/1ns

// Module to configure the MAX2769 frontend via its serial interface.

module frontend_config # (
    parameter SCLK_DIV = 8'd100
) (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic            config_start,   // Start configuration
    output logic            config_busy,    // Configuration busy
    output logic            sclk,           // SPI clock
    output logic            sdata,          // SPI data
    output logic            cs              // SPI chip select
);

localparam logic [27:0] REG_DATA [0:7] = {
    28'hA291973, /*28'hA2919A3,*/   // Filter BW 4.2MHz and centered at 4.26 MHz
    28'h0550288,
    28'hEAFF1DC,
    28'h9EC0008,
    28'h3D62300, /*28'h0C00080,*/   // RDIV 96, NDIV 7857
    28'h8000070,
    28'h8000000,
    28'h10061B2 
};

typedef enum logic [1:0] {
    IDLE,
    DATA,
    ADDR
} serial_state_t;

serial_state_t state, next_state;
logic [7:0] sclk_counter, next_sclk_counter;
logic sclk_oe, next_sclk_oe;
logic sclk_out, next_sclk_out;
assign sclk = sclk_oe ? sclk_out : 1'b0;
logic next_cs;
logic addr, next_addr;
logic [27:0] reg_sdata, next_reg_sdata;
logic [3:0] reg_addr, next_reg_addr;
logic [4:0] bit_counter, next_bit_counter;
logic next_config_busy;

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        state <= IDLE;
        sclk_counter <= SCLK_DIV;
        sclk_out <= 1'b0;
        sclk_oe <= 1'b0;
        cs <= 1'b1;
        reg_sdata <= '0;
        reg_addr <= '0;
        bit_counter <= '0;
        config_busy <= 1'b0;
    end else begin
        state <= next_state;
        sclk_counter <= next_sclk_counter;
        sclk_out <= next_sclk_out;
        sclk_oe <= next_sclk_oe;
        cs <= next_cs;
        reg_sdata <= next_reg_sdata;
        reg_addr <= next_reg_addr;
        bit_counter <= next_bit_counter;
        config_busy <= next_config_busy;
    end
end

always_comb begin
    next_state = state;
    next_sclk_oe = sclk_oe;
    next_sclk_out = sclk_out;
    next_cs = cs;
    next_reg_sdata = reg_sdata;
    next_reg_addr = reg_addr;
    next_bit_counter = bit_counter;
    next_config_busy = config_busy;
    sdata = reg_sdata[27];

    if (sclk_counter == 8'd0) begin
        next_sclk_counter = SCLK_DIV;
        next_sclk_out = ~sclk_out;
    end else begin
        next_sclk_counter = sclk_counter - 8'd1;
    end

    case (state)
        IDLE: begin
            // Align CS going low with SCLK going low
            if ((config_start || reg_addr != 4'd0) && sclk_out && !next_sclk_out) begin
                next_state = DATA;
                next_sclk_oe = 1'b1;
                next_cs = 1'b0;
                next_reg_sdata = REG_DATA[reg_addr[2:0]];
                next_bit_counter = 5'd27;
                next_config_busy = 1'b1;
            end
        end
        DATA: begin
            if (sclk_out && !next_sclk_out) begin
                next_reg_sdata = {reg_sdata[26:0], 1'b0};
                if (bit_counter == 5'd0) begin
                    next_state = ADDR;
                    next_bit_counter = 5'd3;
                    next_reg_sdata = {reg_addr, 24'b0};
                end else begin
                    next_bit_counter = bit_counter - 5'd1;
                end
            end
        end
        ADDR: begin
            if (sclk_out && !next_sclk_out) begin
                next_reg_sdata = {reg_sdata[26:0], 1'b0};
                if (bit_counter == 5'd0) begin
                    next_state = IDLE;
                    next_bit_counter = 5'd0;
                    next_sclk_oe = 1'b0;
                    next_cs = 1'b1;
                    if (reg_addr == 4'd7) begin
                        next_reg_addr = 4'd0;
                        next_config_busy = 1'b0;
                    end else begin
                        next_reg_addr = reg_addr + 4'd1;
                    end
                end else begin
                    next_bit_counter = bit_counter - 5'd1;
                end
            end
        end
        default: begin
            next_state = IDLE;
        end
    endcase
end

endmodule