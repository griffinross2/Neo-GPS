`timescale 1ns/1ns

module sd_cmd (
    input  logic            clk,            // Clock signal
    input  logic            nrst,           // Active low reset
    input  logic [47:0]     cmd_in,         // Command to send (48 bits)
    input  logic            cmd_start,      // Start command transmission
    input  logic            resp_expected,  // 1 if there will be a response to this command
    input  logic            resp_large,     // 1 if response is R2 (136 bits)  
    output logic [135:0]    cmd_resp,       // Response received (up to 136 bits)
    output logic            cmd_resp_valid, // Response valid signal
    inout  wire             cmd             // CMD
);

logic cmd_output, cmd_reg;
logic cmd_tristate, next_cmd_tristate;
assign cmd = cmd_tristate ? 1'bz : cmd_reg;

typedef enum logic [1:0] {
    SD_CMD_BUS_IDLE,
    SD_CMD_BUS_CMD,
    SD_CMD_BUS_WAIT_RESP,
    SD_CMD_BUS_RESP
} sd_cmd_bus_state_t;

sd_cmd_bus_state_t cmd_bus_state, next_cmd_bus_state;

logic resp_expected_reg, next_resp_expected_reg;
logic resp_large_reg, next_resp_large_reg;
logic [135:0] next_cmd_resp;
logic [7:0] cmd_resp_bit_count, next_cmd_resp_bit_count;
logic next_cmd_resp_valid;

always_ff @(posedge clk) begin
    if (~nrst) begin
        cmd_bus_state <= SD_CMD_BUS_IDLE;
        cmd_resp <= '0;
        cmd_resp_bit_count <= 8'd47;
        cmd_resp_valid <= 1'b0;
        resp_expected_reg <= 1'b0;
        resp_large_reg <= 1'b0;
    end else begin
        cmd_bus_state <= next_cmd_bus_state;
        cmd_resp <= next_cmd_resp;
        cmd_resp_bit_count <= next_cmd_resp_bit_count;
        cmd_resp_valid <= next_cmd_resp_valid;
        resp_expected_reg <= next_resp_expected_reg;
        resp_large_reg <= next_resp_large_reg;
    end
end

always_ff @(negedge clk) begin
    if (~nrst) begin
        cmd_reg <= 1'b1;
        cmd_tristate <= 1'b1;
    end else begin
        cmd_reg <= cmd_output;
        cmd_tristate <= next_cmd_tristate;
    end
end

always_comb begin
    // Defaults
    next_cmd_bus_state = cmd_bus_state;
    next_cmd_resp = cmd_resp;
    next_cmd_resp_bit_count = cmd_resp_bit_count;
    next_cmd_tristate = cmd_tristate;
    next_cmd_resp_valid = cmd_resp_valid;
    next_resp_expected_reg = resp_expected_reg;
    next_resp_large_reg = resp_large_reg;
    cmd_output = cmd_resp[47];

    case (cmd_bus_state)
        SD_CMD_BUS_IDLE: begin
            next_cmd_resp_bit_count = 8'd47;

            if (cmd_start) begin
                next_cmd_resp = '0;
                next_cmd_resp[47:0] = cmd_in;
                next_cmd_bus_state = SD_CMD_BUS_CMD;
                next_cmd_tristate = 1'b0;
                next_cmd_resp_valid = 1'b0;
                next_resp_expected_reg = resp_expected;
                next_resp_large_reg = resp_large;
            end
        end
        SD_CMD_BUS_CMD: begin
            // Send CMD MSB first
            next_cmd_resp[47:0] = {cmd_resp[46:0], 1'b0};
            next_cmd_resp_bit_count = cmd_resp_bit_count - 8'd1;

            if (cmd_resp_bit_count == '0) begin
                next_cmd_tristate = 1'b1;
                if (resp_expected_reg) begin
                    next_cmd_bus_state = SD_CMD_BUS_WAIT_RESP;
                end else begin
                    next_cmd_resp_valid = 1'b1;
                    next_cmd_bus_state = SD_CMD_BUS_IDLE;
                end
            end
        end
        SD_CMD_BUS_WAIT_RESP: begin
            // Wait for response start bit (0)
            next_cmd_resp = '0;
            next_cmd_resp_bit_count = resp_large_reg ? 8'd134 : 8'd46;

            if (cmd == 1'b0) begin
                next_cmd_bus_state = SD_CMD_BUS_RESP;
            end
        end
        SD_CMD_BUS_RESP: begin
            // Shift in response bits MSB first
            next_cmd_resp = {cmd_resp[134:0], cmd};
            next_cmd_resp_bit_count = cmd_resp_bit_count - 8'd1;

            if (cmd_resp_bit_count == '0) begin
                next_cmd_bus_state = SD_CMD_BUS_IDLE;
                next_cmd_resp_valid = 1'b1;
            end
        end
        default: begin
            next_cmd_bus_state = SD_CMD_BUS_IDLE;
        end
    endcase
end

endmodule