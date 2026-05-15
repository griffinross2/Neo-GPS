`timescale 1ns/1ns

module sd_data (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic [3:0]      data_in,        // Nibble of data to send
    input  logic            data_start,     // Start data transmission
    inout  wire [3:0]       dat,            // DAT0-3
    output logic            data_next,      // Indicate ready for the next nibble
    output logic            data_done       // Indicate data transfer is complete
);

logic [3:0] dat_output, dat_reg;
logic dat_tristate, next_dat_tristate;
assign dat = dat_tristate ? 4'bz : dat_reg;

typedef enum logic [2:0] {
    SD_DAT_BUS_IDLE,
    SD_DAT_BUS_DAT_START,
    SD_DAT_BUS_DAT,
    SD_DAT_BUS_CRC,
    SD_DAT_BUS_CRC_END,
    SD_DAT_BUS_CRC_STATUS_WAIT,
    SD_DAT_BUS_CRC_STATUS,
    SD_DAT_BUS_WAIT_BUSY
} sd_dat_bus_state_t;

sd_dat_bus_state_t dat_bus_state, next_dat_bus_state;

logic [9:0] data_count, next_data_count;
logic [15:0] dat0_crc;
logic [15:0] dat1_crc;
logic [15:0] dat2_crc;
logic [15:0] dat3_crc;
logic crc_clear, crc_enable;
logic next_data_done;

sd_crc16 sd_crc_inst (
    .clk(clk),
    .nrst(nrst),
    .data_in(data_in),
    .clear(crc_clear),
    .enable(crc_enable),
    .crc_out0(dat0_crc),
    .crc_out1(dat1_crc),
    .crc_out2(dat2_crc),
    .crc_out3(dat3_crc)
);

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        dat_bus_state <= SD_DAT_BUS_IDLE;
        data_count <= 10'd1023;
        data_done <= 1'b0;
    end else begin
        dat_bus_state <= next_dat_bus_state;
        data_count <= next_data_count;
        data_done <= next_data_done;
    end
end

always_ff @(negedge clk, negedge nrst) begin
    if (~nrst) begin
        dat_reg <= '1;
        dat_tristate <= '1;
    end else begin
        dat_reg <= dat_output;
        dat_tristate <= next_dat_tristate;
    end
end

always_comb begin
    // Defaults
    next_dat_bus_state = dat_bus_state;
    next_dat_tristate = dat_tristate;
    next_data_count = data_count;
    dat_output = data_in;
    data_next = 1'b0;
    crc_clear = 1'b0;
    crc_enable = 1'b0;
    next_data_done = data_done;

    case (dat_bus_state)
        SD_DAT_BUS_IDLE: begin
            next_dat_tristate = 1'b1;
            next_data_count = 10'd1023;
            next_data_done = 1'b0;
            if (data_start) begin
                crc_clear = 1'b1;
                next_dat_bus_state = SD_DAT_BUS_DAT_START;
            end
        end
        SD_DAT_BUS_DAT_START: begin
            dat_output = '0;
            next_dat_tristate = 1'b0;
            next_dat_bus_state = SD_DAT_BUS_DAT;
            data_next = 1'b1;
        end
        SD_DAT_BUS_DAT: begin
            dat_output = data_in;
            crc_enable = 1'b1;
            if (data_count == '0) begin
                next_data_count = 10'd15;
                next_dat_bus_state = SD_DAT_BUS_CRC;
            end else begin
                next_data_count = data_count - 10'd1;
                data_next = 1'b1;
            end
        end
        SD_DAT_BUS_CRC: begin
            dat_output[0] = dat0_crc[data_count[3:0]];
            dat_output[1] = dat1_crc[data_count[3:0]];
            dat_output[2] = dat2_crc[data_count[3:0]];
            dat_output[3] = dat3_crc[data_count[3:0]];
            if (data_count == '0) begin
                next_dat_bus_state = SD_DAT_BUS_CRC_END;
            end else begin
                next_data_count = data_count - 10'd1;
            end
        end
        SD_DAT_BUS_CRC_END: begin
            dat_output = '1;
            next_dat_bus_state = SD_DAT_BUS_CRC_STATUS_WAIT;
        end
        SD_DAT_BUS_CRC_STATUS_WAIT: begin
            next_dat_tristate = 1'b1;
            if (dat[0] == 1'b0) begin
                next_dat_bus_state = SD_DAT_BUS_CRC_STATUS;
                next_data_count = 10'd3;
            end
        end
        SD_DAT_BUS_CRC_STATUS: begin
            if (data_count == '0) begin
                next_dat_bus_state = SD_DAT_BUS_WAIT_BUSY;
            end else begin
                next_data_count = data_count - 10'd1;
            end
        end
        SD_DAT_BUS_WAIT_BUSY: begin
            if (dat[0] == 1'b1) begin
                next_data_done = 1'b1;
                next_dat_bus_state = SD_DAT_BUS_IDLE;
            end
        end
        default: begin
            next_dat_bus_state = SD_DAT_BUS_IDLE;
        end
    endcase
end

endmodule