`timescale 1ns/1ns

`include "common_types.vh"
import common_types_pkg::*;

module gps_tb (
);

    logic clk, nrst;
    logic [7:0] spi_data_in;
    logic [7:0] spi_data_out;
    logic [6:0] spi_addr_out;
    logic spi_read;
    logic spi_write;
    logic sck /*verilator public*/;
    logic sdi /*verilator public*/;
    logic sdo /*verilator public*/;
    logic cs /*verilator public*/;

    spi_device spi_dut (
        .clk(clk),
        .nrst(nrst),
        .data_in(spi_data_in),
        .data_out(spi_data_out),
        .addr_out(spi_addr_out),
        .read(spi_read),
        .write(spi_write),
        .sck(sck),
        .sdi(sdi),
        .sdo(sdo),
        .cs(cs)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #26.0417 clk = ~clk;
    end

    logic signal_in;                    // Input signal
    logic start;                        // Start acquisition
    sv_t sv;                            // SV number to search
    word_t acc_out;                     // Maximum correlation
    logic [9:0] code_index;             // Chip index of maximum correlation -> 0 thru 1022
    logic [4:0] start_index;            // Sample start index of maximum correlation -> 0 thru 18
    logic [5:0] dop_index;              // Doppler index of maximum correlation 0 thru 40 -> -5000 thru 5000 Hz in 250 Hz steps
    logic busy;
    logic [5:0] channel_in;
    logic [5:0] channel_out;
    sv_t sv_out;
    logic start_out;

    l1ca_ac_pca_search search_dut (
        .clk(clk),
        .nrst(nrst),
        .start(start),
        .signal_in(signal_in),
        .channel_in(channel_in),
        .channel_out(channel_out),
        .sv(sv),
        .sv_out(sv_out),
        .acc_out(acc_out),
        .code_index(code_index),
        .start_index(start_index),
        .dop_index(dop_index),
        .busy(busy),
        .start_out(start_out)
    );

    // search_ctrl: [7] Busy, [6] Start, [5:0] SV
    logic next_start;
    sv_t next_sv;    
    
    always_ff @(posedge clk or negedge nrst) begin
        if (!nrst) begin
            start <= '0;
            sv <= '0;
        end else begin
            start <= next_start;
            sv <= next_sv;
        end
    end

    always_comb begin
        next_start = start;
        next_sv = sv;

        case (spi_addr_out)
            7'd0: begin
                spi_data_in = {busy, start, sv};
                if (spi_write) begin
                    next_start = spi_data_out[6];
                    next_sv = spi_data_out[5:0];
                end
            end
            default: begin end
        endcase

        if (busy) begin
            next_start = 1'b0;
        end
    end

    integer fd;
    logic [2:0] bit_count;
    logic [7:0] signal_byte;
    initial begin
        fork 
        
        // Signal file feed
        begin
            fd = $fopen("signal.bin", "rb");
            if (fd == 0) begin
                $display("Error opening signal.bin");
                $finish;
            end

            // Read the input signal from the binary file
            while (!$feof(fd)) begin
                @(negedge clk);
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
            channel_in = 0;
            bit_count = 0;

            #200 nrst = 1;

            wait (start == 1'b1);

            $display("Started search: %.2f ns", $realtime());

            @(posedge clk);
            @(posedge clk);
            @(posedge clk);

            wait (busy == 1'b0);

            $display("Finished search: %.2f ns", $realtime());

            #10 $finish; // End simulation
        end

        join
    end

endmodule