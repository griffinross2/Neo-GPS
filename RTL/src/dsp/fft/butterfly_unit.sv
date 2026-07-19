`timescale 1ns/1ns

module butterfly_unit (
    input logic direction,  // 0: forward, 1: inverse
    input logic scaling,    // 0: scale 1-bit per stage, 1: no scaling
    input logic signed [15:0] x0_re, x0_im, x1_re, x1_im,
    input logic signed [15:0] twiddle_re, twiddle_im,
    output logic signed [15:0] y0_re, y0_im, y1_re, y1_im
);

logic signed [31:0] x1_times_twiddle_re, x1_times_twiddle_im;
logic signed [16:0] y0_re_full, y0_im_full, y1_re_full, y1_im_full;
logic signed [15:0] twiddle_im_adj;

always_comb begin
    // Adjust twiddle_im for inverse FFT
    if (direction) begin
        twiddle_im_adj = -twiddle_im;
    end else begin
        twiddle_im_adj = twiddle_im;
    end

    // x1 * twiddle
    x1_times_twiddle_re = x1_re * twiddle_re - x1_im * twiddle_im_adj;
    x1_times_twiddle_im = x1_re * twiddle_im_adj + x1_im * twiddle_re;

    // y0 = x0 + x1 * twiddle
    y0_re_full = {x0_re[15], x0_re} + 17'({x1_times_twiddle_re >>> 15});
    y0_im_full = {x0_im[15], x0_im} + 17'({x1_times_twiddle_im >>> 15});

    // y1 = x0 - x1 * twiddle
    y1_re_full = {x0_re[15], x0_re} - 17'({x1_times_twiddle_re >>> 15});
    y1_im_full = {x0_im[15], x0_im} - 17'({x1_times_twiddle_im >>> 15});

    y0_re = scaling ? 16'(y0_re_full) : y0_re_full[16:1];
    y0_im = scaling ? 16'(y0_im_full) : y0_im_full[16:1];
    y1_re = scaling ? 16'(y1_re_full) : y1_re_full[16:1];
    y1_im = scaling ? 16'(y1_im_full) : y1_im_full[16:1];
end
    
endmodule