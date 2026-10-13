`timescale 1ns/1ns

module parallel_interface_tb;

    localparam CLK_PERIOD = 20; // 50 MHz clock
    localparam IF_CLK_PERIOD = 100; // 10 MHz parallel interface clock

    // Clock and reset signals
    logic clk;
    logic nrst;

    // Parallel interface signals
    logic [31:0] data_in;
    logic [31:0] data_out;
    logic [14:0] addr_out;
    logic read;
    logic write;
    logic if_clk;
    logic if_cs;
    wire [15:0] if_dat;

    logic if_oe;
    logic [15:0] if_dat_out;
    logic [15:0] if_dat_in;
    assign if_dat_in = if_dat;
    assign if_dat = if_oe ? if_dat_out : 16'bz;

    logic [14:0] tb_addr;
    logic [31:0] tb_data;
    always_comb begin
        if (addr_out == tb_addr && read == 1) begin
            data_in = tb_data;
        end else begin
            data_in = 32'b0;
        end
    end

    // Instantiate the parallel_interface module
    parallel_interface dut (
        .clk(clk),
        .nrst(nrst),
        .data_in(data_in),
        .data_out(data_out),
        .addr_out(addr_out),
        .read(read),
        .write(write),
        .if_clk(if_clk),
        .if_cs(if_cs),
        .if_dat(if_dat)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #(CLK_PERIOD/2) clk = ~clk;
    end

    task write_dut (
        input logic [14:0] addr,
        input logic [31:0] data
    );
        if_cs = 0;
        if_dat_out = {1'b0, addr}; // Address phase
        if_oe = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 0;
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 0;
        if_dat_out = data[31:16]; // Data phase 1
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 0;
        if_dat_out = data[15:0]; // Data phase 2
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_cs = 1;
        if_clk = 0;
    endtask

    task read_dut (
        input logic [14:0] addr,
        input logic [31:0] rdata
    );
        tb_addr = addr;
        tb_data = rdata;

        if_cs = 0;
        if_dat_out = {1'b1, addr}; // Address phase
        if_oe = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 0;
        # (IF_CLK_PERIOD/2);
        if_oe = 0;
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 0;
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_clk = 0;
        # (IF_CLK_PERIOD/2);
        if_clk = 1;
        # (IF_CLK_PERIOD/2);
        if_cs = 1;
        if_clk = 0;
        
    endtask

    initial begin
        nrst = 0;
        tb_addr = 0;
        tb_data = 0;
        if_clk = 0;
        if_cs = 1;
        if_oe = 0;
        if_dat_out = 16'b0;

        #20;
        nrst = 1;
        #100;

        write_dut(15'h1234, 32'hDEADBEEF);
        #50;
        write_dut(15'h5678, 32'hCAFEBABE);
        #50;
        read_dut(15'h4321, 32'hFEEDFACE);
        #50;
        read_dut(15'h3412, 32'hBAADF00D);
        #50;

        $finish;
    end

endmodule