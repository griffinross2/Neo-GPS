`timescale 1ns/1ns

`ifdef VIVADO
`include "common_types.vh"
`endif
import common_types_pkg::*;

module l1ca_ac_pca_search_tb;
    logic gps_clk, core_clk, nrst;      // Clock and reset
    logic signal_in;                    // Input signal
    logic start;                        // Start acquisition
    sv_t sv;                            // SV number to search
    word_t acc_out;                     // Maximum correlation
    logic [9:0] code_index;             // Chip index of maximum correlation -> 0 thru 1022
    logic [4:0] start_index;            // Sample start index of maximum correlation -> 0 thru 18
    logic [5:0] dop_index;              // Doppler index of maximum correlation 0 thru 40 -> -5000 thru 5000 Hz in 250 Hz steps
    logic busy;

    l1ca_ac_pca_search dut (
        .gps_clk(gps_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .start(start),
        .signal_in(signal_in),
        .sv(sv),
        .acc_out(acc_out),
        .code_index(code_index),
        .start_index(start_index),
        .dop_index(dop_index),
        .busy(busy)
    );
    initial begin
        gps_clk = 0;
        forever #26.0417 gps_clk = ~gps_clk;
    end

    initial begin
        core_clk = 0;
        forever #10 core_clk = ~core_clk;
    end

    integer fd;
    logic [2:0] bit_count;
    logic [7:0] signal_byte;
    initial begin
        nrst = 0;
        start = 0;
        signal_in = 0;
        sv = 6'd6;
        bit_count = 0;

        #200 nrst = 1;
        
        @(posedge core_clk);
        start = 1;
        @(posedge core_clk);
        start = 0;

        $display("Started search: %.2f ns", $realtime());

        fd = $fopen("signal.bin", "rb");
        if (fd == 0) begin
            $display("Error opening signal.bin");
            $finish;
        end

        // Read the input signal from the binary file
        while (!$feof(fd) && busy) begin
            @(negedge gps_clk);
            if (bit_count == 0) begin
                // Read byte every 8 bits
                signal_byte = $fgetc(fd);
            end

            signal_in = signal_byte[bit_count];
            bit_count = bit_count + 1;
        end

        $fclose(fd);

        wait (busy == 1'b0);

        $display("Finished search: %.2f ns", $realtime());

        #10 $finish; // End simulation
    end

endmodule