`timescale 1ns/1ns

// Peripheral to act as satellite for a custom parallel interface using RPi PIO.

module parallel_interface (
    input  logic                    clk,        // Clock signal
    input  logic                    nrst,       // Active low reset
    input  logic [31:0]             data_in,    // Data to transmit
    output logic [31:0]             data_out,   // Data received
    output logic [14:0]             addr_out,   // Address output (to select data)
    output logic                    read,       // Read register data
    output logic                    write,      // Read register data
    input  logic                    if_clk,    // Parallel interface clock
    input  logic                    if_cs,     // Parallel interface chip select
    inout  wire [15:0]              if_dat     // Parallel interface address/data bus
);

    logic if_oe, next_if_oe;
    logic [15:0] if_dat_out, next_if_dat_out;
    logic [15:0] if_dat_in;

    assign if_dat_in = if_dat;
    assign if_dat = if_oe ? if_dat_out : 16'bz;

    typedef enum logic [2:0] {
        ADDR,
        DUMMY,
        DATA0,
        DATA1,
        WRITE
    } state_t;

    state_t state, next_state;

    // if_clk domain signals from command
    logic [14:0] addr_if_clk, next_addr_if_clk;
    logic read_if_clk, next_read_if_clk;
    logic write_if_clk, next_write_if_clk;

    // clk domain signals from command
    logic [2:0] read_ready_sync;

    // if_clk domain signals for data
    logic [31:0] data_out_if_clk, next_data_out_if_clk;
    
    // clk domain signals for data
    logic [31:0] data_in_clk;
    logic [2:0] write_ready_sync;

    always_ff @(posedge clk, negedge nrst) begin
        if (~nrst) begin
            data_out <= 32'b0;
            addr_out <= 15'b0;
            read <= 1'b0;
            write <= 1'b0;
            read_ready_sync <= 3'b0;
            write_ready_sync <= 3'b0;
            data_in_clk <= 32'b0;
        end else begin
            data_out <= data_out_if_clk;
            addr_out <= addr_if_clk;
            read <= (read_ready_sync[2:1] == 2'b01) ? read_if_clk : 1'b0;
            write <= (write_ready_sync[2:1] == 2'b01) ? write_if_clk : 1'b0;
            read_ready_sync <= {read_ready_sync[1:0], read_if_clk};
            write_ready_sync <= {write_ready_sync[1:0], state == WRITE};
            data_in_clk <= read ? data_in : data_in_clk;
        end
    end

    always_ff @(posedge if_clk, posedge if_cs, negedge nrst) begin
        if (if_cs) begin
            state <= ADDR;
            read_if_clk <= 1'b0;
        end else if (!nrst) begin
            state <= ADDR;
            read_if_clk <= 1'b0;
            addr_if_clk <= 15'b0;
            write_if_clk <= 1'b0;
            data_out_if_clk <= 32'b0;
        end else begin
            state <= next_state;
            addr_if_clk <= next_addr_if_clk;
            read_if_clk <= next_read_if_clk;
            write_if_clk <= next_write_if_clk;
            data_out_if_clk <= next_data_out_if_clk;
        end
    end

    always_ff @(negedge if_clk, posedge if_cs, negedge nrst) begin
        if (if_cs | !nrst) begin
            if_oe <= 1'b0;
            if_dat_out <= 16'b0;
        end else begin
            if_oe <= next_if_oe;
            if_dat_out <= next_if_dat_out;
        end
    end

    always_comb begin
        next_state = state;
        next_if_oe = if_oe;
        next_if_dat_out = if_dat_out;
        next_addr_if_clk = addr_if_clk;
        next_read_if_clk = read_if_clk;
        next_write_if_clk = write_if_clk;
        next_data_out_if_clk = data_out_if_clk;

        case (state)
            ADDR: begin
                if (~if_cs) begin
                    next_addr_if_clk = if_dat_in[14:0];
                    next_read_if_clk = if_dat_in[15];
                    next_write_if_clk = ~if_dat_in[15];
                    next_state = DUMMY;
                end
            end
            DUMMY: begin
                next_state = DATA0;
            end
            DATA0: begin
                next_if_dat_out = data_in_clk[31:16];
                next_data_out_if_clk[31:16] = if_dat_in;
                next_if_oe = read_if_clk ? 1'b1 : 1'b0;
                next_state = DATA1;
            end
            DATA1: begin
                next_if_dat_out = data_in_clk[15:0];
                next_data_out_if_clk[15:0] = if_dat_in;
                next_if_oe = read_if_clk ? 1'b1 : 1'b0;
                next_state = WRITE;
            end
            WRITE: begin
                // Do nothing, CS going high will reset the state machine
            end

            default: begin
                next_state = ADDR;
            end
        endcase
    end

endmodule