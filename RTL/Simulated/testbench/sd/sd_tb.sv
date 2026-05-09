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
wire cmd_inout;
logic cmd;
logic cmd_tristate;

sd_cmd sd_cmd_inst (
    .clk(clk),                          // Clock signal
    .nrst(nrst),                        // Active low reset
    .cmd_in(cmd_in),                    // Command to send (48 bits)
    .cmd_start(cmd_start),              // Start command transmission
    .resp_expected(resp_expected),      // 1 if there will be a response to this command
    .resp_large(resp_large),            // 1 if response is R2 (136 bits)
    .cmd_resp(cmd_resp),                // Response received (up to 136 bits)
    .cmd_resp_valid(cmd_resp_valid),    // Response valid signal
    .cmd(cmd_inout)                     // CMD
);

test PROG (
    .clk(clk),
    .cmd_resp(cmd_resp),
    .cmd_resp_valid(cmd_resp_valid),
    .cmd_tristate(cmd_tristate),
    .cmd_in(cmd_in),
    .cmd_start(cmd_start),
    .resp_expected(resp_expected),
    .resp_large(resp_large),
    .cmd_out(cmd),
    .nrst(nrst)
);

pullup(cmd_inout);
assign cmd_inout = cmd_tristate ? 'z : cmd;

initial begin
    clk = 0;
    forever #2500 clk = ~clk;
end

endmodule

program test (
    input logic clk,
    input logic [135:0] cmd_resp,
    input logic cmd_resp_valid,
    output logic cmd_tristate,
    output logic [47:0] cmd_in,
    output logic cmd_start,
    output logic resp_expected,
    output logic resp_large,
    output logic cmd_out,
    output logic nrst
);

    task reset_dut();
        cmd_in = 48'h1234_5678_9ABC;
        cmd_start = 0;
        resp_large = 0;
        resp_expected = 0;
        cmd_out = 0;
        cmd_tristate = 1;

        nrst = 0;
        @(negedge clk);
        @(negedge clk);
        nrst = 1;

        @(negedge clk);
        @(negedge clk);
    endtask

    task send_command(input logic [47:0] cmd_in_val, input logic resp_expected_val, input logic resp_large_val);
        integer i;
        logic[135:0] resp;

        cmd_in = cmd_in_val;
        resp_expected = resp_expected_val;
        resp_large = resp_large_val;
        
        // Start command
        cmd_start = 1;
        @(negedge clk);
        cmd_start = 0;

        // Wait for command to send
        for (i = 0; i < 52; i++) begin
            @(negedge clk);
        end

        if (resp_expected_val) begin
            // Send a response
            cmd_tristate = 0;
            for (i = 0; i < 48; i++) begin
                resp = (136'h7ABC_DEF1_234F >> (47 - i)) & 136'b1;
                cmd_out = resp[0];
                @(negedge clk);
            end
            cmd_tristate = 1;
            cmd_out = 0;

            wait(cmd_resp_valid);
            @(negedge clk);
            $display("Received response: %h", cmd_resp);
        end
    endtask

    initial begin
        reset_dut();

        send_command(48'h1234_5678_9ABC, 1, 0); // Send command with expected response

        send_command(48'hABCD_EF12_3456, 0, 0); // Send command with no expected response

        $finish;
    end
endprogram