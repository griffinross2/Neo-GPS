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

    l1ca_ac_pca_search search_dut (
        .clk(clk),
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
        spi_data_in = 8'h00;

        case (spi_addr_out)
            // Search control register
            7'd0: begin
                spi_data_in = {busy, start, sv};
                if (spi_write) begin
                    next_start = spi_data_out[6];
                    next_sv = spi_data_out[5:0];
                end
            end

            // Search results - Accumulator
            7'd1: begin
                spi_data_in = acc_out[7:0];
            end
            7'd2: begin
                spi_data_in = acc_out[15:8];
            end
            7'd3: begin
                spi_data_in = acc_out[23:16];
            end
            7'd4: begin
                spi_data_in = acc_out[31:24];
            end

            // Search results - code index, start index, doppler index
            7'd5: begin
                // Code index [7:0]
                spi_data_in = code_index[7:0];
            end
            7'd6: begin
                // [7:6] - Code index [9:8]
                // [5]   - reserved
                // [4:0] - start index [4:0]
                spi_data_in = {code_index[9:8], 1'b0, start_index[4:0]};
            end
            7'd7: begin
                // [7:6] - reserved
                // [5:0] - doppler index [5:0]
                spi_data_in = {2'b00, dop_index[5:0]};
            end
            default: begin 
                spi_data_in = 8'h00;
            end
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
            bit_count = 0;

            #200 nrst = 1;

            wait (start == 1'b1);

            $display("Started search: %.2f ns", $realtime());

            @(posedge clk);
            @(posedge clk);
            @(posedge clk);

            wait (busy == 1'b0);

            $display("Finished search: %.2f ns", $realtime());

            #10ms $finish; // End simulation
        end

        join
    end

endmodule