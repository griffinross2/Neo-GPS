`timescale 1ns/1ns

module sd_top (
    input CLK, ck_rst,
    output LED [0:3],
    input BTN[0:3],
    input ja_0,
    output ja_2,
    inout ja_1, ja_3, ja_4, ja_6, ja_7,
    input jb_0, jb_1, jb_2, jb_3, jb_6,
    output jb_4, jb_5, jb_7
);

    localparam CLK_DIV_INIT = 8'd124;
    localparam CLK_DIV = 8'd1;

    wire [3:0] dat;
    wire cmd;
    wire nrst;
    wire cd;
    wire sd_clk;
    logic crc_error;
    logic fifo_overrun;

    assign dat = {ja_1, ja_4, ja_3, ja_7};
    assign cmd = ja_6;
    assign nrst = ck_rst;
    assign cd = ja_0;
    assign sd_clk = ja_2;

    wire i0, i1, q0, q1;
    wire gnss_clk;
    wire cs, dout, sck;
    assign gnss_clk = jb_6;
    assign i0 = jb_2;
    assign i1 = jb_3;
    assign q0 = jb_0;
    assign q1 = jb_1;
    assign jb_4 = dout;
    assign jb_5 = sck;
    assign jb_7 = cs;
    // assign {sck, dout, cs} = 3'b100;

    logic [2:0] record_button_sync;
    logic record;
    always_ff @(posedge gnss_clk, negedge nrst) begin
        if (~nrst) begin
            record_button_sync <= 3'b000;
            record <= 1'b0;
        end else begin
            record_button_sync <= {record_button_sync[1:0], BTN[0]};
            if (&record_button_sync) begin
                record <= 1'b1;
            end
        end
    end
    assign LED[0] = record;
    assign LED[1] = crc_error;
    assign LED[2] = fifo_overrun;

    logic config_start;
    logic config_busy;

    always_ff @(posedge CLK, negedge nrst) begin
        if (~nrst) begin
            config_start <= 1'b1;
        end else begin
            if (config_busy) begin
                config_start <= 1'b0;
            end
        end
    end

    assign LED[3] = config_busy;

    frontend_config frontend_config_inst (
        .clk(CLK),
        .nrst(nrst),
        .config_start(config_start),
        .config_busy(config_busy),
        .sclk(sck),
        .cs(cs),
        .sdata(dout)
    );

    sd_host #(
        .CLK_DIV_INIT(CLK_DIV_INIT),
        .CLK_DIV(CLK_DIV)
    ) sd_host_inst (
        .clk(CLK),                      // Clock signal
        .nrst(nrst),                    // Active low reset
        .gnss_clk(gnss_clk),            // GPS sample clock
        .sample_bits({q1, q0, i1, i0}), // Sample bits from the GPS receiver
        .init(1'b1),                    // Initialize the SD card
        .record(record),                // Start recording
        .cmd(cmd),                      // CMD
        .dat(dat),                      // DAT0-3
        .sd_clk(sd_clk),                // SD clock
        .crc_error(crc_error),          // CRC error output
        .fifo_overrun(fifo_overrun)     // FIFO overrun output
    );

endmodule