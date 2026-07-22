/**************************************************************************************/
/*                        Averaging Correlation PCA Search                            */
/*                             Using 4096-length FFT                                  */
/*                                   Based on:                                        */
/*                                                                                    */
/*                             J. A. Starzyk and Z. Zhu,                              */
/* "Averaging correlation for C/A code acquisition and tracking in frequency domain," */
/*    Proceedings of the 44th IEEE 2001 Midwest Symposium on Circuits and Systems.    */
/*                 MWSCAS 2001 (Cat. No.01CH37257), Dayton, OH, USA,                  */
/*                            2001, pp. 905-908 vol.2,                                */
/*                        doi: 10.1109/MWSCAS.2001.986334.                            */
/**************************************************************************************/

`include "common_types.vh"
import common_types_pkg::*;

module l1ca_ac_pca_search (
    input logic clk, nrst,                      // Clock and reset
    input logic signal_in,                      // Input signal
    input logic start,                          // Start acquisition
    input sv_t sv,                              // SV number to search
    input logic [5:0] channel_in,               // Channel number to initialize after acquisition
    output word_t acc_out,                      // Maximum correlation
    output logic [9:0] code_index,              // Chip index of maximum correlation -> 0 thru 1022
    output logic [4:0] start_index,             // Sample start index of maximum correlation -> 0 thru 18
    output logic [5:0] dop_index,               // Doppler index of maximum correlation 0 thru 40 -> -5000 thru 5000 Hz in 250 Hz steps
    output logic [5:0] channel_out,             // Channel number to initialize after acquisition
    output sv_t sv_out,                         // SV number to initialize after acquisition
    output logic start_out,                     // Start signal for the channel
    output logic busy                           // Busy signal
);

typedef enum logic [2:0] {
    IDLE,
    SAMPLE,
    CODE_FFT,
    SAMPLE_FFT,
    PRODUCT_IFFT
} state_t;

typedef enum logic [1:0] {
    FFT_CONF,
    FFT_LOAD,
    FFT_WAIT
} fft_state_t;

state_t state, next_state;
fft_state_t fft_state, next_fft_state;

logic fft_start;
logic fft_direction;
logic fft_scaling;
logic fft_ready;
logic fft_done;
logic [15:0] fft_x_re, fft_x_im;
logic [15:0] fft_X_re, fft_X_im;

fft_4096 fft_inst (
    .clk(clk),
    .nrst(nrst),
    .start(fft_start),
    .direction(fft_direction),
    .scaling(fft_scaling),
    .data_ready(fft_ready),
    .done(fft_done),
    .x_re(fft_x_re),
    .x_im(fft_x_im),
    .X_re(fft_X_re),
    .X_im(fft_X_im)
);

logic [16:0] sample_addr, next_sample_addr;
logic [11:0] sample_fft_addr, next_sample_fft_addr;
logic [11:0] code_fft_addr, next_code_fft_addr;
logic [11:0] code_fft_addr_override;
logic signed [5:0] sample_i_avg, next_sample_i_avg, sample_i_avg_plus_sample;
logic signed [5:0] sample_q_avg, next_sample_q_avg, sample_q_avg_plus_sample;
logic sample_downsample_i, sample_downsample_q;
logic sample_i_in;
logic sample_q_in;
logic sample_i_out;
logic sample_q_out;
logic sample_wen;
logic sample_ren;
logic sample_fft_wen;
logic sample_fft_ren;
logic code_fft_wen;
logic code_fft_ren;
logic signed [15:0] sample_fft_i_out, sample_fft_q_out;
logic signed [15:0] code_fft_i_out, code_fft_q_out;
logic signed [31:0] si_ci_prod, sq_cq_prod, si_cq_prod, sq_ci_prod;
logic signed [5:0] doppler_step, next_doppler_step; // We will step from -20 to 20
logic [4:0] start_step, next_start_step; // Start step from 0 to 18
word_t acc_magnitude;
word_t next_acc_out;
logic [9:0] next_code_index;
logic [4:0] next_start_index;
logic [5:0] next_dop_index;
logic [5:0] next_channel_out;
sv_t next_sv_out;
logic next_start_out;

// Code NCOs
logic [31:0] code_phase;
logic [32:0] next_code_phase;
localparam CODE_RATE = 33'd228841226;

// LO NCO
logic [17:0] lo_phase;
logic [17:0] next_lo_phase;
localparam logic [17:0] LO_RATE = 18'd54886;

localparam LO_SIN = 4'b0011;
localparam LO_COS = 4'b1001;

// Sample memory
spram #(
    .ADDR_WIDTH(17),
    .RAM_DEPTH(76800), // 4ms at 19200000 Hz
    .DATA_WIDTH(1)
) sample_i (
    .clka(clk),
    .ena(sample_wen | sample_ren),
    .wea(sample_wen),
    .addra(sample_addr),
    .dia(sample_i_in),
    .doa(sample_i_out)
);
spram #(
    .ADDR_WIDTH(17),
    .RAM_DEPTH(76800), // 4ms at 19200000 Hz
    .DATA_WIDTH(1)
) sample_q (
    .clka(clk),
    .ena(sample_wen | sample_ren),
    .wea(sample_wen),
    .addra(sample_addr),
    .dia(sample_q_in),
    .doa(sample_q_out)
);

// Sample FFT results
spram #(
    .ADDR_WIDTH(12),
    .RAM_DEPTH(4096),
    .DATA_WIDTH(32)
) sample_fft (
    .clka(clk),
    .ena(sample_fft_wen | sample_fft_ren),
    .wea(sample_fft_wen),
    .addra(sample_fft_addr),
    .dia({fft_X_im, fft_X_re}),
    .doa({sample_fft_q_out, sample_fft_i_out})
);

// Code FFT results
spram #(
    .ADDR_WIDTH(12),
    .RAM_DEPTH(4096),
    .DATA_WIDTH(32)
) code_fft (
    .clka(clk),
    .ena(code_fft_wen | code_fft_ren),
    .wea(code_fft_wen),
    .addra(code_fft_addr),
    .dia({fft_X_im, fft_X_re}),
    .doa({code_fft_q_out, code_fft_i_out})
);

logic code_strobe;
logic code_clear;
logic code;
logic epoch;
logic [9:0] code_num;

// Code generator
l1ca_code code_gen (
    .clk(clk),
    .nrst(nrst),
    .en(code_strobe),
    .clear(code_clear),
    .sv(sv),
    .code(code),
    .chip(code_num),
    .epoch(epoch)
);

always_ff @(posedge clk) begin
    if (~nrst) begin
        state <= IDLE;
        sample_addr <= '0;
        sample_fft_addr <= '0;
        code_fft_addr <= '0;
        sample_i_avg <= '0;
        sample_q_avg <= '0;
        code_phase <= '0;
        lo_phase <= '0;
        fft_state <= FFT_CONF;
        doppler_step <= -6'd20;
        start_step <= '0;
        acc_out <= '0;
        code_index <= '0;
        start_index <= '0;
        dop_index <= '0;
        channel_out <= '0;
        sv_out <= '0;
        start_out <= 1'b0;
    end else begin
        state <= next_state;
        sample_addr <= next_sample_addr;
        sample_fft_addr <= next_sample_fft_addr;
        code_fft_addr <= next_code_fft_addr;
        sample_i_avg <= next_sample_i_avg;
        sample_q_avg <= next_sample_q_avg;
        code_phase <= next_code_phase[31:0];
        lo_phase <= next_lo_phase;
        fft_state <= next_fft_state;
        doppler_step <= next_doppler_step;
        start_step <= next_start_step;
        acc_out <= next_acc_out;
        code_index <= next_code_index;
        start_index <= next_start_index;
        dop_index <= next_dop_index;
        channel_out <= next_channel_out;
        sv_out <= sv;
        start_out <= next_start_out;
    end
end

always_comb begin
    next_state = state;
    next_fft_state = fft_state;
    next_sample_addr = sample_addr;
    next_sample_fft_addr = sample_fft_addr;
    next_code_fft_addr = code_fft_addr;
    code_fft_addr_override = code_fft_addr;
    next_sample_i_avg = sample_i_avg;
    next_sample_q_avg = sample_q_avg;
    sample_i_avg_plus_sample = sample_i_avg + (sample_i_out ? 6'd1 : -6'd1);
    sample_q_avg_plus_sample = sample_q_avg + (sample_q_out ? 6'd1 : -6'd1);
    sample_i_in = signal_in ^ (LO_SIN[lo_phase[17:16]]);
    sample_q_in = signal_in ^ (LO_COS[lo_phase[17:16]]);
    sample_wen = 1'b0;
    sample_ren = 1'b0;
    sample_fft_wen = 1'b0;
    sample_fft_ren = 1'b0;
    code_fft_wen = 1'b0;
    code_fft_ren = 1'b0;
    sample_downsample_i = ~sample_i_avg_plus_sample[5];
    sample_downsample_q = ~sample_q_avg_plus_sample[5];
    next_doppler_step = doppler_step;
    next_acc_out = acc_out;
    next_code_index = code_index;
    next_start_index = start_index;
    next_dop_index = dop_index;
    next_start_step = start_step;
    acc_magnitude = {{16{fft_X_re[15]}}, fft_X_re[15:0]}*{{16{fft_X_re[15]}}, fft_X_re[15:0]} + 
                    {{16{fft_X_im[15]}}, fft_X_im[15:0]}*{{16{fft_X_im[15]}}, fft_X_im[15:0]};

    next_code_phase = {1'b0, code_phase};
    next_lo_phase = lo_phase;
    next_channel_out = channel_out;
    next_sv_out = sv_out;
    next_start_out = 1'b0;

    busy = 1'b1;

    code_strobe = 1'b0;
    code_clear = 1'b0;

    fft_start = '0;
    fft_direction = '0;
    fft_scaling = '0;
    fft_x_re = '0;
    fft_x_im = '0;
    fft_ready = '1;

    si_ci_prod = {{16{sample_fft_i_out[15]}}, sample_fft_i_out} * {{16{code_fft_i_out[15]}}, code_fft_i_out};
    sq_cq_prod = {{16{sample_fft_q_out[15]}}, sample_fft_q_out} * {{16{code_fft_q_out[15]}}, code_fft_q_out};
    si_cq_prod = {{16{sample_fft_i_out[15]}}, sample_fft_i_out} * {{16{code_fft_q_out[15]}}, code_fft_q_out};
    sq_ci_prod = {{16{sample_fft_q_out[15]}}, sample_fft_q_out} * {{16{code_fft_i_out[15]}}, code_fft_i_out};

    case (state)
        IDLE: begin
            busy = 1'b0;
            if (start) begin
                next_sample_addr = '0;
                next_sample_fft_addr = '0;
                next_code_fft_addr = '0;
                next_sample_i_avg = '0;
                next_sample_q_avg = '0;
                next_code_phase = '0;
                code_clear = 1'b1;
                next_doppler_step = -6'd20;
                next_start_step = '0;
                next_acc_out = '0;
                next_code_index = '0;
                next_start_index = '0;
                next_dop_index = '0;
                next_channel_out = channel_in;
                next_sv_out = sv;
                next_state = SAMPLE;
            end
        end
        SAMPLE: begin
            next_lo_phase = lo_phase + LO_RATE;

            // Write the sample
            next_sample_addr = sample_addr + 17'd1;
            sample_wen = 1'b1;

            if (sample_addr == 76799) begin
                next_state = CODE_FFT;
                next_fft_state = FFT_CONF;
                next_sample_addr = '0;
                next_code_fft_addr = '0;
            end
        end
        CODE_FFT: begin
            if (fft_state == FFT_CONF) begin
                fft_start = 1'b1;
                fft_direction = 1'b0; // Forward FFT
                fft_scaling = 1'b0;
                next_fft_state = FFT_LOAD;
                code_fft_ren = 1'b1; // Start reading code
                next_code_fft_addr = code_fft_addr + 12'd1;
            end
            if (fft_state == FFT_LOAD) begin
                code_fft_ren = 1'b1; // Start reading code
                fft_x_re = code ? 16'h7FFF : 16'h8001;
                fft_x_im = 16'd0;
                code_strobe = 1'b1;
                next_code_fft_addr = code_fft_addr + 12'd1;
                if (code_fft_addr == '0) begin
                    next_code_fft_addr = '0;
                    next_fft_state = FFT_WAIT;
                end
            end
            if (fft_state == FFT_WAIT) begin
                if (fft_done) begin
                    code_fft_wen = 1'b1;
                    next_code_fft_addr = code_fft_addr + 12'd1;
                    if (code_fft_addr == 12'hFFF) begin
                        next_code_fft_addr = '0;
                        next_sample_fft_addr = '0;
                        next_state = SAMPLE_FFT;
                        next_fft_state = FFT_CONF;
                    end
                end
            end
        end
        SAMPLE_FFT: begin
            if (fft_state == FFT_CONF) begin
                fft_start = 1'b1;
                fft_direction = 1'b0; // Forward FFT
                fft_scaling = 1'b0;
                next_fft_state = FFT_LOAD;
                sample_ren = 1'b1; // Start reading samples
                next_sample_addr = sample_addr + 17'd1;
                next_sample_fft_addr = '0;
                next_code_phase = '0;
            end
            if (fft_state == FFT_LOAD) begin
                fft_ready = 0;
                sample_ren = 1'b1;
                next_code_phase = code_phase + CODE_RATE;
                fft_x_re = sample_downsample_i ? 16'h7FFF : 16'h8001;
                fft_x_im = sample_downsample_q ? 16'h7FFF : 16'h8001;
                next_sample_addr = (sample_addr + 17'd1) % 76800;
                next_sample_i_avg = sample_i_avg + (sample_i_out ? 6'd1 : -6'd1);
                next_sample_q_avg = sample_q_avg + (sample_q_out ? 6'd1 : -6'd1);
                
                if (next_code_phase[32]) begin
                    // Finish this averaging period
                    fft_ready = 1;
                
                    next_sample_i_avg = '0;
                    next_sample_q_avg = '0;
                    next_sample_addr = (sample_addr + 17'd1) % 76800;
                    next_sample_fft_addr = sample_fft_addr + 12'd1;

                    if (sample_fft_addr == 12'hFFF) begin
                        next_sample_addr = '0;
                        next_sample_fft_addr = '0;
                        next_fft_state = FFT_WAIT;
                    end
                end
            end
            if (fft_state == FFT_WAIT) begin
                if (fft_done) begin
                    sample_fft_wen = 1'b1;
                    next_sample_fft_addr = sample_fft_addr + 12'd1;
                    if (sample_fft_addr == 12'hFFF) begin
                        next_sample_fft_addr = '0;
                        next_code_fft_addr = 12'd0 - {{6{next_doppler_step[5]}}, next_doppler_step};
                        next_state = PRODUCT_IFFT;
                        next_fft_state = FFT_CONF;
                    end
                end
            end
        end
        PRODUCT_IFFT: begin
            if (fft_state == FFT_CONF) begin
                fft_start = 1'b1;
                fft_direction = 1'b1; // Inverse FFT
                fft_scaling = 1'b0;
                next_fft_state = FFT_LOAD;
                sample_fft_ren = 1'b1; // Start reading sample FFT
                code_fft_ren = 1'b1; // Start reading code FFT
                next_sample_fft_addr = sample_fft_addr + 12'd1;
                next_code_fft_addr = code_fft_addr + 12'd1;
            end
            if (fft_state == FFT_LOAD) begin
                sample_fft_ren = 1'b1;
                code_fft_ren = 1'b1;
                fft_x_re = 16'(si_ci_prod >>> 8) + 16'(sq_cq_prod >>> 8);
                fft_x_im = 16'(sq_ci_prod >>> 8) - 16'(si_cq_prod >>> 8);
                next_sample_fft_addr = sample_fft_addr + 12'd1;
                next_code_fft_addr = code_fft_addr + 12'd1;
                if (sample_fft_addr == '0) begin
                    next_sample_fft_addr = '0;
                    next_code_fft_addr = '0;
                    next_fft_state = FFT_WAIT;
                end
            end
            if (fft_state == FFT_WAIT) begin
                if (fft_done) begin
                    next_sample_fft_addr = sample_fft_addr + 12'd1;
                    // Look through the results
                    if (sample_fft_addr < 1023) begin
                        // Check for maximum
                        if (acc_magnitude > acc_out) begin
                            next_acc_out = acc_magnitude;
                            next_code_index = sample_fft_addr[9:0];
                            next_start_index = start_step;
                            next_dop_index = doppler_step;
                        end
                    end

                    if (sample_fft_addr == 12'hFFF) begin
                        // Initial indices for sample and code FFTs
                        next_doppler_step = doppler_step + 6'd1;
                        next_sample_fft_addr = '0;
                        next_code_fft_addr = 12'd0 - {{6{next_doppler_step[5]}}, next_doppler_step};
                        next_state = PRODUCT_IFFT;
                        next_fft_state = FFT_CONF;

                        if (doppler_step == 6'd20) begin
                            // We have finished all Doppler steps, now we need to increment the start step
                            next_doppler_step = -6'd20;
                            next_start_step = start_step + 5'd2;    // 0->18 in steps of 2 equals 10 points within code chip (plenty)
                            next_sample_addr = {12'd0, next_start_step};
                            next_state = SAMPLE_FFT;

                            if (start_step == 5'd18) begin
                                next_state = IDLE;
                                next_start_out = 1'b1; // Signal channel to start
                            end
                        end
                    end
                end
            end
        end
        default: begin
            next_state = IDLE;
        end
    endcase
end

endmodule
