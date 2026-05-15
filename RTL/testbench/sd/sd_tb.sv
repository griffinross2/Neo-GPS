`timescale 1ns/1ns

module sd_tb (
);

logic clk, gnss_clk;
logic nrst;
logic [3:0] sample_bits;
logic record;
wire cmd;
wire [3:0] dat;
wire sd_clk;

sd_host sd_host_inst (
    .clk(clk),                      // Clock signal
    .nrst(nrst),                    // Active low reset
    .gnss_clk(gnss_clk),            // GPS sample clock
    .sample_bits(sample_bits),      // Sample bits from the GPS receiver
    .init(1'b1),                    // Initialize the SD card
    .record(record),                // Start recording
    .cmd(cmd),                      // CMD
    .dat(dat),                      // DAT0-3
    .sd_clk(sd_clk)                 // SD clock
);

// test PROG (
//     .clk(clk),
//     .cmd_resp(cmd_resp),
//     .cmd_resp_valid(cmd_resp_valid),
//     .cmd_tristate(cmd_tristate),
//     .cmd_in(cmd_in),
//     .cmd_start(cmd_start),
//     .resp_expected(resp_expected),
//     .resp_large(resp_large),
//     .cmd_out(cmd),
//     .nrst(nrst)
// );

initial begin
    gnss_clk = 0;
    forever #26.041667 gnss_clk = ~gnss_clk; // 19.2 MHz
end

logic clk_50, clk_100, clk_200;
logic opt_enable_hs;

sd_top sd_inst (
    .clk_50(clk_50),
    .clk_100(clk_100),
    .clk_200(clk_200),
    .reset_n(nrst),
    .sd_clk(sd_clk),
    .sd_cmd(cmd),
    .sd_dat(dat),
    .opt_enable_hs(opt_enable_hs)
);

pullup(cmd);
pullup(dat[0]);
pullup(dat[1]);
pullup(dat[2]);
pullup(dat[3]);

initial begin
    clk_50 = 0;
    forever #10 clk_50 = ~clk_50;       // 50 MHz
end

initial begin
    clk_100 = 0;
    forever #5 clk_100 = ~clk_100;      // 100 MHz
end

initial begin
    clk_200 = 0;
    forever #2.5 clk_200 = ~clk_200;    // 200 MHz
end

assign clk = clk_100;

initial begin
    sample_bits = 4'b1010;
    record = 1'b0;
    nrst = 1;
    #1;
    nrst = 0;
    #5000;
    nrst = 1;

    #20000000;

    record = 1'b1;

    #10000000;

    $finish;
end

endmodule