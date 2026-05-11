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
    .start(1'b0),       // Start recording
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

    #20000000;

    $finish;
end


endmodule

// program test (
//     input logic clk,
//     input logic [135:0] cmd_resp,
//     input logic cmd_resp_valid,
//     output logic cmd_tristate,
//     output logic [47:0] cmd_in,
//     output logic cmd_start,
//     output logic resp_expected,
//     output logic resp_large,
//     output logic cmd_out,
//     output logic nrst
// );

//     task reset_dut();
//         cmd_in = 48'h1234_5678_9ABC;
//         cmd_start = 0;
//         resp_large = 0;
//         resp_expected = 0;
//         cmd_out = 0;
//         cmd_tristate = 1;

//         nrst = 0;
//         @(negedge clk);
//         @(negedge clk);
//         nrst = 1;

//         @(negedge clk);
//         @(negedge clk);
//     endtask

//     task send_command(input logic [47:0] cmd_in_val, input logic resp_expected_val, input logic resp_large_val);
//         integer i;
//         logic[135:0] resp;

//         cmd_in = cmd_in_val;
//         resp_expected = resp_expected_val;
//         resp_large = resp_large_val;
        
//         // Start command
//         cmd_start = 1;
//         @(negedge clk);
//         cmd_start = 0;

//         // Wait for command to send
//         for (i = 0; i < 52; i++) begin
//             @(negedge clk);
//         end

//         if (resp_expected_val) begin
//             // Send a response
//             cmd_tristate = 0;
//             for (i = 0; i < 48; i++) begin
//                 resp = (136'h7ABC_DEF1_234F >> (47 - i)) & 136'b1;
//                 cmd_out = resp[0];
//                 @(negedge clk);
//             end
//             cmd_tristate = 1;
//             cmd_out = 0;

//             wait(cmd_resp_valid);
//             @(negedge clk);
//             $display("Received response: %h", cmd_resp);
//         end
//     endtask

//     initial begin
//         reset_dut();

//         send_command(48'h1234_5678_9ABC, 1, 0); // Send command with expected response

//         send_command(48'hABCD_EF12_3456, 0, 0); // Send command with no expected response

//         $finish;
//     end
// endprogram