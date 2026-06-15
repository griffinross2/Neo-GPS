`timescale 1ns/1ns

module frontend_config_tb (
);

logic   clk;            // Clock signal
logic   nrst;           // Active low reset
logic   config_start;   // Start configuration
logic   config_busy;    // Configuration busy
logic   sclk;           // SPI clock
logic   sdata;          // SPI data
logic   cs;             // SPI chip select

frontend_config frontend_inst (
    .clk(clk),                      // Clock signal
    .nrst(nrst),                    // Active low reset
    .config_start(config_start),    // Start configuration
    .config_busy(config_busy),      // Configuration busy
    .sclk(sclk),                    // SPI clock
    .sdata(sdata),                  // SPI data
    .cs(cs)                         // SPI chip select
);

initial begin
    clk = 0;
    forever #5 begin
        clk = ~clk; // 100 MHz
    end
end

initial begin
    config_start = 1'b0;
    nrst = 1;
    #1;
    nrst = 0;
    #100;
    nrst = 1;

    #100;

    config_start = 1'b1;
    wait(config_busy);
    wait(!config_busy);

    #1000;

    $finish;
end

endmodule