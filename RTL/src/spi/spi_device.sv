`timescale 1ns/1ns

// Multi-address reads/writes are NOT allowed

module spi_device #(
    parameter DATA_WIDTH = 8
) (
    input  logic                    clk,        // Clock signal
    input  logic                    nrst,       // Active low reset
    input  logic [DATA_WIDTH-1:0]   data_in,    // Data to transmit
    output logic [DATA_WIDTH-1:0]   data_out,   // Data received
    output logic [DATA_WIDTH-2:0]   addr_out,   // Address output (to select data)
    output logic                    read,       // Read register data
    output logic                    write,      // Read register data
    output logic                    sck,        // SPI clock
    input  logic                    sdi,        // Serial data in
    output logic                    sdo,        // Serial data out
    input  logic                    cs          // Chip select (active low)
);

    localparam DATA_WIDTH_LOG2 = $clog2(DATA_WIDTH);

    // States
    typedef enum logic {
        ADDR,   // Address
        DATA    // Send data
    } state_t;
    
    state_t state, next_state;
    logic [DATA_WIDTH_LOG2-1:0] bit_counter, next_bit_counter;
    logic [DATA_WIDTH-2:0] next_addr_out;
    logic [DATA_WIDTH-1:0] next_data_out;
    logic readwrite, next_readwrite;
    logic read_sck;
    logic write_sck;
    logic [DATA_WIDTH-1:0] data_read_reg_clk;
    logic [DATA_WIDTH-1:0] data_read_reg_sck, next_data_read_reg_sck;

    assign sdo = data_read_reg_sck[DATA_WIDTH-1];
    
    always_ff @(negedge sck or posedge cs or negedge nrst) begin
        if (cs || !nrst) begin
            state <= ADDR;
            bit_counter <= '0;
            data_read_reg_sck <= 0;
        end else begin
            state <= next_state;
            bit_counter <= next_bit_counter;
            data_read_reg_sck <= next_data_read_reg_sck;
        end
    end

    always_ff @(posedge sck or negedge nrst) begin
        if (!nrst) begin
            data_out <= '0;
            addr_out <= '0;
            readwrite <= 0;
        end else begin
            data_out <= next_data_out;
            addr_out <= next_addr_out;
            readwrite <= next_readwrite;
        end
    end

    always_comb begin
        next_state = state;
        next_bit_counter = bit_counter;
        next_addr_out = addr_out;
        next_data_out = data_out;
        next_readwrite = readwrite;
        read_sck = 0;
        write_sck = 0;
        next_data_read_reg_sck = data_read_reg_clk;

        case (state)
            ADDR: begin
                if (bit_counter == 0) begin
                    // First bit is read/write bit
                    next_readwrite = sdi;
                end else begin
                    // Rest of the bits are address bits
                    next_addr_out = {addr_out[DATA_WIDTH-3:0], sdi};
                end

                if (bit_counter == DATA_WIDTH_LOG2'(DATA_WIDTH-1)) begin
                    // All bits received, move to DATA state
                    next_bit_counter = '0;
                    next_state = DATA;

                    if (readwrite && sck) begin
                        // Trigger a read from the system.
                        read_sck = 1;
                    end

                end else begin
                    next_bit_counter = bit_counter + 1;
                end
            end
            DATA: begin
                next_bit_counter = bit_counter + 1;

                if (bit_counter == DATA_WIDTH_LOG2'(DATA_WIDTH-1)) begin

                    if (!readwrite && sck) begin
                        // Trigger a write to the system.
                        write_sck = 1;
                    end

                    // All bits received, go to ADDR state
                    next_bit_counter = '0;
                    next_state = ADDR;
                end else begin
                    // Shift the data out until its time to reload it from the CLK domain
                    next_data_read_reg_sck = {data_read_reg_sck[DATA_WIDTH-2:0], 1'b0};
                end

                next_data_out = {data_out[DATA_WIDTH-2:0], sdi};
            end
        endcase
    end

    logic [2:0] read_sync;
    logic [2:0] write_sync;

    always_ff @(posedge clk or negedge nrst) begin
        if (!nrst) begin
            read_sync <= 0;
            read <= 0;
            write_sync <= 0;
            write <= 0;
            data_read_reg_clk <= 0;
        end else begin
            read_sync = {read_sync[1:0], read_sck};
            read <= read_sync == 3'b011;
            write_sync = {write_sync[1:0], write_sck};
            write <= write_sync == 3'b011;
            data_read_reg_clk <= (read ? data_in : data_read_reg_clk);
        end
    end

endmodule