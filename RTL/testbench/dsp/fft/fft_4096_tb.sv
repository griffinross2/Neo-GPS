`timescale 1ns/1ns

module fft_4096_tb (
);

logic clk, nrst;
logic start;
logic direction;
logic scaling;
logic data_ready;
logic done;

logic signed [15:0] x_re, x_im;
logic signed [15:0] X_re, X_im;

logic [31:0] test_inputs [0:4095];
logic [31:0] test_outputs [0:4095];

initial begin
    $readmemh("fft_test_input.hex", test_inputs);
end

fft_4096 dut (
    .clk(clk),
    .nrst(nrst),
    .start(start),
    .direction(direction),
    .scaling(scaling),
    .data_ready(data_ready),
    .done(done),
    .x_re(x_re),
    .x_im(x_im),
    .X_re(X_re),
    .X_im(X_im)
);

initial begin
    clk = 0;
    forever #5 begin
        clk = ~clk; // 100 MHz
    end
end

initial begin
    start = 0;
    direction = 0;
    scaling = 0;
    data_ready = 1;
    x_re = 16'sd0;
    x_im = 16'sd0;
    
    nrst = 0;
    #20;
    nrst = 1;
    #20;

    @(negedge clk);
    start = 1;
    @(negedge clk);
    start = 0;
    for (int i = 0; i < 4096; i++) begin
        data_ready = 1;
        x_re = test_inputs[i][31:16];
        x_im = test_inputs[i][15:0];
        @(negedge clk);
        data_ready = 0;
        @(negedge clk);
    end

    wait(done);
    @(negedge clk);
    for (int i = 0; i < 4096; i++) begin
        test_outputs[i] = {X_re, X_im};
        @(negedge clk);
    end

    #20;
    
    $writememh("fft_test_output.hex", test_outputs);

    $finish;
end

endmodule