`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

module gps_tb (
);

    logic gps_clk, core_clk, nrst;
    logic signal_in;
    logic sck /*verilator public*/;
    logic sdi /*verilator public*/;
    logic sdo /*verilator public*/;
    logic cs /*verilator public*/;

    gps dut (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(signal_in),
        .sck(sck),
        .sdi(sdi),
        .sdo(sdo),
        .cs(cs)
    );

    // Clock generation
    initial begin
        gps_clk = 0;
        forever #26 gps_clk = ~gps_clk;
    end

    initial begin
        core_clk = 0;
        forever #10 core_clk = ~core_clk;
    end

    integer fd;
    logic [2:0] bit_count;
    logic [7:0] signal_byte;
    initial begin
        fork 
        
        // Signal file feed
        begin
            wait (dut.search0_start_gps == 1'b1);

            fd = $fopen("signal.bin", "rb");
            if (fd == 0) begin
                $display("Error opening signal.bin");
                $finish;
            end

            // Read the input signal from the binary file
            while (!$feof(fd)) begin
                @(negedge gps_clk);
                if (bit_count == 0) begin
                    // Read byte every 8 bits
                    signal_byte = $fgetc(fd);
                end

                signal_in = signal_byte[bit_count];
                bit_count = bit_count + 1;
            end

            $fclose(fd);
        end

        // Testbench start and end
        begin
            nrst = 0;
            signal_in = 0;
            bit_count = 0;

            #200 nrst = 1;

            wait (dut.search0_start_gps == 1'b1);

            $display("Started search: %.2f ns", $realtime());

            @(posedge gps_clk);
            @(posedge gps_clk);
            @(posedge gps_clk);

            wait (dut.search_unit_0.search_busy == 1'b0);

            $display("Finished search: %.2f ns", $realtime());
        end

        join
    end

endmodule