`timescale 1ns/1ns

`include "common_types.vh"
import common_types_pkg::*;

module l1ca_channel (
    input logic clk, nrst,                      // Clock and reset
    input logic start, clear,                   // Start the channel
    input sv_t sv,                              // SV number for code generation
    input logic [31:0] code_rate, lo_rate,      // Code and local oscillator rates
    input logic signal_in,                      // Signal input
    input logic pause,                          // Pause the channel
    input logic [15:0] pause_delay,             // Pause delay to start tracking
    output logic epoch,                         // Epoch signal
    output gps_chip_t chip,                     // Chip index from code generator
    output acc_t ie, qe,                        // Early in-phase and quadrature phase output accumulator
    output acc_t ip, qp,                        // Prompt in-phase and quadrature phase output accumulator
    output acc_t il, ql,                        // Late in-phase and quadrature phase output accumulator
    output logic [31:0] code_phase, lo_phase    // Code and local oscillator phases
);

    typedef enum logic {
        IDLE,       // Idle state
        ACTIVE      // Tracking
    } channel_state_t;

    channel_state_t state, next_state;

    logic code_epoch, epoch_reg;

    acc_t next_ie, next_qe; // Next state for accumulators
    acc_t next_ip, next_qp; // Next state for accumulators
    acc_t next_il, next_ql; // Next state for accumulators
    logic die, dip, dil;    // in-phase DLL signals
    logic dqe, dqp, dql;    // quadrature phase DLL signals

    logic [32:0] next_code_phase;   // Next state for code phase + overflow bit
    logic code_strobe;      // Code strobe from NCO
    logic delay_strobe;     // Delay strobe from NCO (twice rate of code strobe)

    logic code_early, code_prompt, code_late; // Code samples for early, prompt, and late
    logic next_code_prompt, next_code_late; // Next state for code samples

    logic [31:0] next_lo_phase; // Next state for local oscillator phase
    logic lo_cos, lo_sin;       // Local oscillator outputs

    logic [15:0] pause_delay_reg, next_pause_delay_reg; // Pause delay register

    always_ff @(posedge clk, negedge nrst) begin
        if (~nrst) begin
            ip <= '0;
            qp <= '0;
            ie <= '0;
            qe <= '0;
            il <= '0;
            ql <= '0;
            code_prompt <= '0;
            code_late <= '0;
            epoch_reg <= '0;
            state <= IDLE;
            code_phase <= '0;
            lo_phase <= '0;
            pause_delay_reg <= '0;
        end else begin
            ip <= next_ip;
            qp <= next_qp;
            ie <= next_ie;
            qe <= next_qe;
            il <= next_il;
            ql <= next_ql;
            code_late <= next_code_late;
            code_prompt <= next_code_prompt;
            epoch_reg <= code_epoch;
            state <= next_state;
            code_phase <= next_code_phase[31:0];
            lo_phase <= next_lo_phase;
            pause_delay_reg <= next_pause_delay_reg;
        end
    end

    always_comb begin
        next_state = state;

        case (state)
            IDLE: begin
                if (start && !clear) begin
                    next_state = ACTIVE;
                end
            end

            ACTIVE: begin
                if (clear) begin
                    next_state = IDLE;
                end
            end

            default: begin
                next_state = IDLE;
            end
        endcase
    end

    // State output
    always_comb begin
        next_ip = ip;
        next_qp = qp;
        next_ie = ie;
        next_qe = qe;
        next_il = il;
        next_ql = ql;
        next_code_prompt = code_prompt;
        next_code_late = code_late;
        next_pause_delay_reg = pause_delay_reg;

        if (pause) begin
            next_pause_delay_reg = pause_delay;
        end else begin
            if (pause_delay_reg != 0) begin
                next_pause_delay_reg = pause_delay_reg - 16'd1;
            end
        end

        // Code NCO
        code_strobe = 1'b0;
        delay_strobe = 1'b0;

        // Don't do anything if paused
        if (pause_delay_reg == '0) begin
            next_code_phase = {1'b0, code_phase} + {1'b0, code_rate};

            // Overflow of code phase should trigger the code strobe
            if (next_code_phase[32]) begin
                code_strobe = 1'b1;
            end

            // Delay strobe is twice the rate of code strobe
            if (next_code_phase[31] ^ code_phase[31]) begin
                delay_strobe = 1'b1;
            end
        end

        // Local oscillator NCO
        next_lo_phase = lo_phase + lo_rate;
        lo_cos = 1'b0;
        lo_sin = 1'b0;

        // Generate local oscillator output
        case (lo_phase[31:30])
            2'b00: begin
                lo_cos = 1'b1;
                lo_sin = 1'b1;
            end
            2'b01: begin
                lo_cos = 1'b0;
                lo_sin = 1'b1;
            end
            2'b10: begin
                lo_cos = 1'b0;
                lo_sin = 1'b0;
            end
            2'b11: begin
                lo_cos = 1'b1;
                lo_sin = 1'b0;
            end
        endcase

        // Mixers
        die = code_early ^ signal_in ^ lo_sin;
        dip = code_prompt ^ signal_in ^ lo_sin;
        dil = code_late ^ signal_in ^ lo_sin;
        dqe = code_early ^ signal_in ^ lo_cos;
        dqp = code_prompt ^ signal_in ^ lo_cos;
        dql = code_late ^ signal_in ^ lo_cos;

        case (state)
            IDLE: begin
                next_ip = '0;
                next_qp = '0;
                next_ie = '0;
                next_qe = '0;
                next_il = '0;
                next_ql = '0;
                next_code_prompt = '0;
                next_code_late = '0;
                next_code_phase = '0;
                next_lo_phase = '0;
            end

            ACTIVE: begin
                if (delay_strobe) begin
                    // Shift code samples
                    next_code_late = code_prompt;
                    next_code_prompt = code_early;
                end

                // Accumulate in-phase and quadrature components
                next_ie = ie + (die ? 32'd1 : -32'd1);
                next_qe = qe + (dqe ? 32'd1 : -32'd1);
                next_ip = ip + (dip ? 32'd1 : -32'd1);
                next_qp = qp + (dqp ? 32'd1 : -32'd1);
                next_il = il + (dil ? 32'd1 : -32'd1);
                next_ql = ql + (dql ? 32'd1 : -32'd1);

                // On rising edge of epoch, clear the accumulators
                if (code_epoch & ~epoch_reg) begin
                    next_ie = '0;
                    next_qe = '0;
                    next_ip = '0;
                    next_qp = '0;
                    next_il = '0;
                    next_ql = '0;
                end
            end
        endcase
        
        if (clear) begin
            next_ip = '0;
            next_qp = '0;
            next_ie = '0;
            next_qe = '0;
            next_il = '0;
            next_ql = '0;
            next_code_prompt = '0;
            next_code_late = '0;
            next_code_phase = '0;
            next_lo_phase = '0;
        end

        // Rising edge of code epoch, assert epoch output
        epoch = (state == ACTIVE) & ~pause & code_epoch & ~epoch_reg;
    end

    // Code generator
    l1ca_code code_gen (
        .clk(clk),
        .nrst(nrst),
        .en(code_strobe),
        .clear(state == IDLE),
        .sv(sv),
        .code(code_early),
        .epoch(code_epoch),
        .chip(chip)
    );

endmodule