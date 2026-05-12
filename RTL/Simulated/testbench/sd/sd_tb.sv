`timescale 1ns/1ns

module sd_tb (
);

logic clk;
logic nrst;
logic [47:0] cmd_in;
logic cmd_start;
logic resp_expected;
logic resp_large;
logic [135:0] cmd_resp;
logic cmd_resp_valid;

wire cmd;
wire [3:0] dat;
wire sd_clk;

sd_host sd_host_inst (
    .clk(clk),          // Clock signal
    .nrst(nrst),        // Active low reset
    .init(1'b1),        // Initialize the SD card
    .record(1'b1),      // Start recording
    .cmd(cmd),          // CMD
    .dat(dat),          // DAT0-3
    .sd_clk(sd_clk)     // SD clock
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
    clk = 0;
    forever #20 clk = ~clk;
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
    clk_100 = 0;
    clk_200 = 0;
    forever begin
        #10 clk_50 = ~clk_50;       // 50 MHz
        #5 clk_100 = ~clk_100;      // 100 MHz
        #2.5 clk_200 = ~clk_200;    // 200 MHz
    end
end

initial begin
    nrst = 1;
    #1;
    nrst = 0;
    #5000;
    nrst = 1;

    #30000000;

    $finish;
end

endmodule