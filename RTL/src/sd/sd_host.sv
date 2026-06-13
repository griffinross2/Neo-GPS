`timescale 1ns/1ns

module sd_host #(
    parameter CLK_DIV_INIT = 8'd124,
    parameter CLK_DIV = 8'd1
)
(
    input   logic           clk,            // Clock signal
    input   logic           nrst,           // Active low reset
    input   logic           gnss_clk,       // GPS sample clock
    input   logic [3:0]     sample_bits,    // Sample bits from the GPS receiver
    input   logic           init,           // Initialize the SD card
    input   logic           record,         // Start recording
    inout   wire            cmd,            // CMD
    inout   wire [3:0]      dat,            // DAT0-3
    output  wire            sd_clk,         // SD clock
    output  logic           crc_error,      // Indicate CRC error in data transfer
    output  logic           fifo_overrun    // Indicate that the FIFO was not written to the SD card fast enough
);

typedef enum logic [3:0] {
    SD_HOST_RESET_WAIT,
    SD_HOST_INIT,
    SD_HOST_CMD0,
    SD_HOST_CMD8,
    SD_HOST_CMD55_ACMD41,
    SD_HOST_ACMD41,
    SD_HOST_CMD2,
    SD_HOST_CMD3,
    SD_HOST_CMD7,
    SD_HOST_CMD55_ACMD6,
    SD_HOST_ACMD6,
    SD_HOST_TRANSFER,
    SD_HOST_CMD25,
    SD_HOST_CMD25_DATA,
    SD_HOST_CMD12
} sd_host_state_t;

sd_host_state_t host_state, next_host_state;
logic [6:0] reset_clock_count, next_reset_clock_count;
logic [15:0] rca, next_rca;
logic [2:0] cmd_interval_countdown, next_cmd_interval_countdown;

logic [38:0] cmd_to_send;
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

logic [31:0] data_block_addr, next_data_block_addr;
logic [3:0] data_to_send;
logic data_start;
logic data_next;
logic data_done;

sd_data sd_data_inst (
    .clk(sd_clk),
    .nrst(nrst),
    .data_in(data_to_send),
    .data_start(data_start),
    .dat(dat),
    .data_next(data_next),
    .data_done(data_done),
    .crc_error(crc_error)
);

logic fifo_wr_en;
logic [18:0] fifo_wr_addr;
logic [18:0] fifo_rd_addr, next_fifo_rd_addr;
logic fifo_block_ready, next_fifo_block_ready;
logic [1:0] fifo_block_ready_sync;
logic fifo_block_started, next_fifo_block_started;
logic [1:0] fifo_block_started_sync;
logic [2:0] fifo_block_buffer;
logic [1:0] fifo_block_buffer_idx;
logic next_fifo_overrun;

dpram #(
    .RAM_DEPTH(524288),
    .ADDR_WIDTH(19)
) fifo_inst (
    .clka(~gnss_clk),
    .clkb(sd_clk),
    .ena(1'b1),
    .enb(1'b1),
    .wea(fifo_wr_en),
    .addra(fifo_wr_addr),
    .addrb(fifo_rd_addr),
    .dia({sample_bits[1], fifo_block_buffer}),
    .dob(data_to_send)
);

always_ff @(negedge gnss_clk, negedge nrst) begin
    if (~nrst) begin
        fifo_wr_en <= 1'b0;
        fifo_wr_addr <= '0;
        fifo_block_ready <= 1'b0;
        fifo_block_started_sync <= 2'b00;
        fifo_block_buffer <= '0;
        fifo_block_buffer_idx <= 2'b00;
        fifo_overrun <= 1'b0;
    end else begin
        if (record) begin
            fifo_wr_en <= 1'b1;
        end
        if (fifo_wr_en) begin
            if (fifo_block_buffer_idx == 2'b11) begin
                fifo_wr_addr <= fifo_wr_addr + 19'd1;
            end
            fifo_block_buffer_idx <= fifo_block_buffer_idx + 2'd1;
        end
        fifo_block_ready <= next_fifo_block_ready;
        fifo_block_started_sync <= {fifo_block_started_sync[0], fifo_block_started};
        fifo_block_buffer <= {sample_bits[1], fifo_block_buffer[2:1]};
        fifo_overrun <= next_fifo_overrun;
    end
end

always_comb begin
    next_fifo_block_ready = fifo_block_ready;
    next_fifo_overrun = fifo_overrun;
    if (fifo_wr_en && (fifo_wr_addr == 19'h3FFFF || fifo_wr_addr == 19'h7FFFF)) begin
        next_fifo_block_ready = 1'b1;
        // The SD card state should already be back in TRANSFER, else we will overrun
        if (!fifo_block_ready && host_state != SD_HOST_TRANSFER) begin
            next_fifo_overrun = 1'b1;
        end
    end else if (&fifo_block_started_sync) begin
        next_fifo_block_ready = 1'b0;
    end
end

logic sd_clk_reg;
logic [7:0] clk_divider, next_clk_divider;
logic [7:0] clk_counter;
assign sd_clk = sd_clk_reg;

always_ff @(posedge clk, negedge nrst) begin
    if (~nrst) begin
        clk_counter <= CLK_DIV_INIT;
        sd_clk_reg <= 1'b0;
    end else begin
        if (clk_counter == '0) begin
            sd_clk_reg <= ~sd_clk_reg;
            clk_counter <= clk_divider;
        end else begin
            clk_counter <= clk_counter - 8'd1;
        end
        
    end
end
 
always_ff @(posedge sd_clk_reg, negedge nrst) begin
    if (~nrst) begin
        clk_divider <= CLK_DIV_INIT;
        host_state <= SD_HOST_RESET_WAIT;
        reset_clock_count <= 7'd74;
        cmd_start <= 1'b0;
        rca <= '0;
        fifo_block_ready_sync <= 2'b00;
        fifo_block_started <= 1'b0;
        fifo_rd_addr <= '0;
        data_block_addr <= '0;
        cmd_interval_countdown <= '1;
    end else begin
        clk_divider <= next_clk_divider;
        host_state <= next_host_state;
        reset_clock_count <= next_reset_clock_count;
        cmd_start <= next_cmd_start;
        rca <= next_rca;
        fifo_block_ready_sync <= {fifo_block_ready_sync[0], fifo_block_ready};
        fifo_block_started <= next_fifo_block_started;
        fifo_rd_addr <= next_fifo_rd_addr;
        data_block_addr <= next_data_block_addr;
        cmd_interval_countdown <= next_cmd_interval_countdown;
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
    data_start = 1'b0;
    next_rca = rca;
    next_fifo_block_started = 1'b0;
    next_data_block_addr = data_block_addr;
    next_cmd_interval_countdown = cmd_interval_countdown;

    next_fifo_rd_addr = fifo_rd_addr;
    if(data_next) begin
        next_fifo_rd_addr = fifo_rd_addr + 19'd1;
    end

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
            cmd_to_send = {1'b1, 6'd0, 32'b0}; // CMD0
            next_cmd_start = 1'b0;
            resp_expected = 1'b0;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_CMD8;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_CMD8: begin
            cmd_to_send = {1'b1, 6'd8, 20'b0, 4'b0001, 8'b10101010}; // CMD8, 2.7-3.6V, check pattern
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_CMD55_ACMD41;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_CMD55_ACMD41: begin
            cmd_to_send = {1'b1, 6'd55, 32'b0}; // CMD55
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_ACMD41;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_ACMD41: begin
            cmd_to_send = {1'b1, 6'd41, 1'b0, 1'b1, 1'b0, 1'b1, 3'b0, 1'b0, 24'h100000}; // ACMD41, HCS=1, XPC=1, S18R=0, Set 3.2-3.3 voltage window bit
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_cmd_start = 1'b1;
                if (~cmd_response[39]) begin // Check if card is busy
                    next_host_state = SD_HOST_CMD55_ACMD41;
                end else begin
                    next_host_state = SD_HOST_CMD2;
                end
            end
        end
        SD_HOST_CMD2: begin
            cmd_to_send = {1'b1, 6'd2, 32'b0}; // CMD2
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            resp_large = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_CMD3;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_CMD3: begin
            cmd_to_send = {1'b1, 6'd3, 32'b0}; // CMD3
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_CMD7;
                next_cmd_start = 1'b1;
                next_rca = cmd_response[39:24];
            end
        end
        SD_HOST_CMD7: begin
            cmd_to_send = {1'b1, 6'd7, rca, 16'b0}; // CMD7 with RCA
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_CMD55_ACMD6;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_CMD55_ACMD6: begin
            cmd_to_send = {1'b1, 6'd55, rca, 16'b0}; // CMD55
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_ACMD6;
                next_cmd_start = 1'b1;
            end
        end
        SD_HOST_ACMD6: begin
            cmd_to_send = {1'b1, 6'd6, 30'b0, 2'b10}; // ACMD6, set bus width to 4 bits
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0) begin
                next_host_state = SD_HOST_TRANSFER;
                next_clk_divider = CLK_DIV;
            end
        end
        SD_HOST_TRANSFER: begin
            if (&fifo_block_ready_sync) begin
                next_cmd_start = 1'b1;
                next_host_state = SD_HOST_CMD25;
                next_fifo_block_started = 1'b1;
            end
        end
        SD_HOST_CMD25: begin
            next_fifo_block_started = 1'b1;
            cmd_to_send = {1'b1, 6'd25, data_block_addr}; // CMD25
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd5) begin
                next_data_block_addr = data_block_addr + 32'd256;
                next_host_state = SD_HOST_CMD25_DATA;
            end
        end
        SD_HOST_CMD25_DATA: begin
            data_start = 1'b1;
            if (data_done && (fifo_rd_addr == 19'h0 || fifo_rd_addr == 19'h40000)) begin
                data_start = 1'b0;
                next_cmd_start = 1'b1;
                next_host_state = SD_HOST_CMD12;
            end
        end
        SD_HOST_CMD12: begin
            cmd_to_send = {1'b1, 6'd12, 32'b0}; // CMD12
            next_cmd_start = 1'b0;
            resp_expected = 1'b1;
            if (cmd_interval_countdown == 3'd0 && dat[0]) begin
                next_host_state = SD_HOST_TRANSFER;
            end
        end
        default: begin
            next_host_state = host_state;
        end
    endcase

    // Delay between commands
    if (cmd_resp_valid && ~cmd_start && |cmd_interval_countdown) begin
        next_cmd_interval_countdown = cmd_interval_countdown - 3'd1;
    end
    if (next_cmd_start) begin
        next_cmd_interval_countdown = '1;
    end
end

endmodule