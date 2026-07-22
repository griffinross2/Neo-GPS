`timescale 1ns/1ns

module control_unit (
    input logic clk, nrst,
    
    input logic start,
    input logic data_ready,
    output logic done,
    
    output logic [1:0] mem_wsrc,
    output logic mem_we,
    output logic [11:0] mem_addr,

    output logic [10:0] tw_idx,

    output logic bfly_load
);

logic next_done;

typedef enum logic [2:0] {
    IDLE,
    LOAD,
    BUTTERFLY_READ_0,
    BUTTERFLY_READ_1,
    BUTTERFLY_READ_2,
    BUTTERFLY_WRITE_0,
    BUTTERFLY_WRITE_1,
    OUTPUT
} state_t;

state_t state, next_state;

logic [11:0] span, next_span;
logic [3:0] stage_counter, next_stage_counter;
logic [11:0] addr_counter, next_addr_counter;
logic [11:0] bit_reverse_addr;

always_ff @(posedge clk or negedge nrst) begin
    if (!nrst) begin
        state <= IDLE;
        stage_counter <= '0;
        addr_counter <= '0;
        span <= '0;
        done <= '0;
    end else begin
        state <= next_state;
        stage_counter <= next_stage_counter;
        addr_counter <= next_addr_counter;
        span <= next_span;
        done <= next_done;
    end
end

always_comb begin
    next_state = state;
    next_stage_counter = stage_counter;
    next_addr_counter = addr_counter;
    next_span = span;
    next_done = done;
    bit_reverse_addr = { << {addr_counter}};

    mem_addr = bit_reverse_addr;
    mem_wsrc = 2'd0;
    mem_we = 1'b0;

    tw_idx = 11'((addr_counter & (span - 12'd1)) << (11-stage_counter));

    bfly_load = 1'b0;

    case (state)
        IDLE: begin
            if (start) begin
                next_stage_counter = '0;
                next_addr_counter = '0;
                next_state = LOAD;
                next_done = 1'b0;
            end
        end
        LOAD: begin
            mem_addr = bit_reverse_addr;
            mem_wsrc = 2'd0;

            if (data_ready) begin
                mem_we = 1'b1;
                next_addr_counter = addr_counter + 1;

                if (&addr_counter) begin
                    next_span = 1;
                    next_addr_counter = '0;
                    next_state = BUTTERFLY_READ_0;
                end
            end
        end
        BUTTERFLY_READ_0: begin
            // Get first input from RAM, don't register for butterfly yet
            mem_addr = ((addr_counter >> stage_counter) << (stage_counter + 1)) | (addr_counter & (span - 12'd1));
            bfly_load = 1'b0;

            next_state = BUTTERFLY_READ_1;
        end
        BUTTERFLY_READ_1: begin
            // Get second input from RAM register first input for butterfly
            mem_addr = ((addr_counter >> stage_counter) << (stage_counter + 1)) | (addr_counter & (span - 12'd1)) | span;
            bfly_load = 1'b1;

            next_state = BUTTERFLY_READ_2;
        end
        BUTTERFLY_READ_2: begin
            // Register second input for butterfly
            bfly_load = 1'b1;

            next_state = BUTTERFLY_WRITE_0;
        end
        BUTTERFLY_WRITE_0: begin
            mem_addr = ((addr_counter >> stage_counter) << (stage_counter + 1)) | (addr_counter & (span - 12'd1));
            mem_we = 1'b1;
            mem_wsrc = 2'd1;

            next_state = BUTTERFLY_WRITE_1;
        end
        BUTTERFLY_WRITE_1: begin
            mem_addr = ((addr_counter >> stage_counter) << (stage_counter + 1)) | (addr_counter & (span - 12'd1)) | span;
            mem_we = 1'b1;
            mem_wsrc = 2'd2;

            next_state = BUTTERFLY_READ_0;
            next_addr_counter = addr_counter + 1;

            if (&addr_counter[10:0]) begin
                // Next stage
                next_addr_counter = '0;
                next_span = span << 1;
                next_stage_counter = stage_counter + 1;
                if (span[11]) begin
                    // Done
                    mem_addr = '0;
                    next_addr_counter = 1;
                    next_state = OUTPUT;
                    next_done = 1'b1;
                end
            end
        end
        OUTPUT: begin
            mem_addr = addr_counter;
            next_addr_counter = addr_counter + 1;

            if (&addr_counter) begin
                next_state = IDLE;
            end
        end
    endcase
end

endmodule