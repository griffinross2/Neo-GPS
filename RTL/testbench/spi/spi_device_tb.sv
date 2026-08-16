`timescale 1ns/1ns

module spi_device_tb (
);
    logic clk, nrst;
    logic [15:0] data_in;
    logic [15:0] data_out;
    logic [14:0] addr_out;
    logic read;
    logic write;
    logic sck;
    logic sdi;
    logic sdo;
    logic cs;
    logic sck_en;
    logic sck_gated;

    assign sck_gated = sck & sck_en;
    assign data_in = {1'b1, addr_out};

    spi_device dut (
        .clk(clk),
        .nrst(nrst),
        .data_in(data_in),
        .data_out(data_out),
        .addr_out(addr_out),
        .read(read),
        .write(write),
        .sck(sck_gated),
        .sdi(sdi),
        .sdo(sdo),
        .cs(cs)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    initial begin
        sck = 0;
        forever #43 sck = ~sck;
    end

    task reset_dut();
        cs = 1;
        sck_en = 0;
        nrst = 0;
        @(negedge clk);
        @(negedge clk);
        nrst = 1;
    endtask

    task send_spi_data(
        input logic [15:0] data,
        input logic keep_cs,
        integer bit_counter = 0
    );
        cs = 0;
        for (bit_counter = 0; bit_counter < 16; bit_counter = bit_counter + 1) begin
            sck_en = 1;
            sdi = data[15 - bit_counter];
            @(negedge sck);
        end
        if (!keep_cs) begin
            @(posedge sck);
            sck_en = 0;
            cs = 1;
            @(negedge sck);
        end
    endtask

    initial begin
        reset_dut();

        @(negedge sck);
        @(negedge sck);
        @(negedge sck);

        // Read from 0x2513
        send_spi_data(16'hA513, 1);
        send_spi_data(16'h0000, 0);

        // Write to 0x3162
        send_spi_data(16'h3162, 1);
        send_spi_data(16'h67F3, 0);

        // Read from 0x7492
        send_spi_data(16'hF492, 1);
        send_spi_data(16'h0000, 0);

        // Write to 0x395A
        send_spi_data(16'h395A, 1);
        send_spi_data(16'h1862, 0);

        @(negedge sck);
        @(negedge sck);

        $finish;
    end

endmodule

