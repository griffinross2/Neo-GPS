`timescale 1ns/1ns

module sd_host (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic            init,           // Initialize the SD card
    input  logic            start,          // Start recording
    inout  wire             cmd,            // CMD
    inout  wire [3:0]       dat,            // DAT0-3
    output wire             sd_clk          // SD clock
);

logic [47:0] cmd_to_send;
logic [135:0] cmd_response;
logic cmd_resp_valid;
logic cmd_start, next_cmd_start;
logic resp_expected;
logic resp_large;

sd_cmd sd_cmd_inst (
    .clk(sd_clk),
    .nrst(nrst),
    .cmd_in(cmd_to_send),
    .cmd_start(cmd_start),
    .resp_expected(resp_expected),
    .resp_large(resp_large),
    .cmd_resp(cmd_response),
    .cmd_resp_valid(cmd_resp_valid),
    .cmd(cmd)
);

logic [3:0] data_to_send;
logic data_start;
logic data_next;

sd_data sd_data_inst (
    .clk(sd_clk),
    .nrst(nrst),
    .data_in(data_to_send),
    .data_start(data_start),
    .dat(dat),
    .data_next(data_next)
);

localparam logic [7:0] CLK_DIV_INIT = 8'd62;
localparam logic [7:0] CLK_DIV = 8'd62;
logic sd_clk_reg;
logic [7:0] clk_divider, next_clk_divider;
logic [7:0] clk_counter;
assign sd_clk = sd_clk_reg;

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        clk_divider <= CLK_DIV_INIT;
        clk_counter <= CLK_DIV_INIT;
        sd_clk_reg <= 1'b0;
    end else begin
        clk_divider <= next_clk_divider;
        if (clk_counter == '0) begin
            sd_clk_reg <= ~sd_clk_reg;
            clk_counter <= clk_divider;
        end else begin
            clk_counter <= clk_counter - 8'd1;
        end
    end
end

typedef enum logic [3:0] {
    SD_HOST_RESET_WAIT,
    SD_HOST_INIT,
    SD_HOST_CMD0,
    SD_HOST_CMD8,
    SD_HOST_CMD55,
    SD_HOST_ACMD41,
    SD_HOST_CMD2,
    SD_HOST_CMD3,
    SD_HOST_CMD7,
    SD_HOST_TRANSFER,
    SD_HOST_CMD24
} sd_host_state_t;

sd_host_state_t host_state, next_host_state;

logic [6:0] reset_clock_count, next_reset_clock_count;

always_ff @(posedge sd_clk_reg, negedge nrst) begin
    if (~nrst) begin
        host_state <= SD_HOST_RESET_WAIT;
        reset_clock_count <= 7'd74;
        cmd_start <= 1'b0;
    end else begin
        host_state <= next_host_state;
        reset_clock_count <= next_reset_clock_count;
        cmd_start <= next_cmd_start;
    end
end

always_comb begin
    // Defaults
    next_reset_clock_count = reset_clock_count;
    next_host_state = host_state;
    next_clk_divider = clk_divider;
    next_cmd_start = cmd_start;
    cmd_to_send = '0;
    resp_expected = 1'b1;
    resp_large = 1'b0;
    data_to_send = '0;
    data_start = 1'b0;

    case (host_state)
        SD_HOST_RESET_WAIT: begin
            next_reset_clock_count = reset_clock_count - 7'd1;
            if (reset_clock_count == '0) begin
                next_host_state = SD_HOST_INIT;
            end
        end
        SD_HOST_INIT: begin
            if (init) begin
                next_host_state = SD_HOST_CMD0;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_CMD0: begin
            cmd_to_send = {1'b0, 1'b1, 6'd0, 32'b0, 7'h4A, 1'b1}; // CMD0 with CRC
            next_cmd_start = 1'b0;
            resp_expected = 1'b0;
            if (cmd_resp_valid) begin
                next_host_state = SD_HOST_CMD8;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_CMD8: begin
            cmd_to_send = {1'b0, 1'b1, 6'd8, 20'b0, 4'b0001, 8'b10101010, 7'h43, 1'b1}; // CMD8 with CRC
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_resp_valid) begin
                next_host_state = SD_HOST_CMD8;
                next_cmd_start = 1'b1;
            end
        end
        default: begin
            next_host_state = SD_HOST_INIT;
        end
    endcase
end

endmodule