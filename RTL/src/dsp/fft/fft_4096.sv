`timescale 1ns/1ns

module fft_4096 (
    input logic clk, nrst,
    
    input logic start,
    input logic direction,  // 0: forward, 1: inverse
    input logic scaling,    // 0: scale 1-bit per stage, 1: no scaling
    input logic data_ready,
    output logic done,

    input logic signed [15:0] x_re, x_im,
    output logic signed [15:0] X_re, X_im
);

logic dir_reg;
logic scaling_reg;
logic [1:0] mem_wsrc;
logic mem_we;
logic [11:0] mem_addr;
logic [10:0] tw_idx;
logic bfly_load;
logic signed [15:0] mem_wdata_re, mem_wdata_im;
logic signed [15:0] mem_rdata_re, mem_rdata_im;
logic signed [15:0] tw_re, tw_im;
logic signed [15:0] butterfly_input0_re, butterfly_input0_im;
logic signed [15:0] butterfly_input1_re, butterfly_input1_im;
logic signed [15:0] butterfly_output0_re, butterfly_output0_im;
logic signed [15:0] butterfly_output1_re, butterfly_output1_im;

always_ff @(posedge clk or negedge nrst) begin
    if (!nrst) begin
        butterfly_input0_re <= '0;
        butterfly_input0_im <= '0;
        butterfly_input1_re <= '0;
        butterfly_input1_im <= '0;
        dir_reg <= 1'b0;
        scaling_reg <= 1'b0;
    end else begin
        if (bfly_load) begin
            butterfly_input1_re <= mem_rdata_re;
            butterfly_input1_im <= mem_rdata_im;
            butterfly_input0_re <= butterfly_input1_re;
            butterfly_input0_im <= butterfly_input1_im;
        end
        if (start) begin
            dir_reg <= direction;
            scaling_reg <= scaling;
        end
    end
end

always_comb begin
    mem_wdata_re = x_re;
    mem_wdata_im = x_im;
    
    if (mem_wsrc == 2'd1) begin
        mem_wdata_re = butterfly_output0_re;
        mem_wdata_im = butterfly_output0_im;
    end else if (mem_wsrc == 2'd2) begin
        mem_wdata_re = butterfly_output1_re;
        mem_wdata_im = butterfly_output1_im;
    end

    X_re = mem_rdata_re;
    X_im = mem_rdata_im;
end

control_unit cu (
    .clk(clk),
    .nrst(nrst),
    .start(start),
    .data_ready(data_ready),
    .done(done),
    .mem_wsrc(mem_wsrc),
    .mem_we(mem_we),
    .mem_addr(mem_addr),
    .tw_idx(tw_idx),
    .bfly_load(bfly_load)
);

twiddle_unit tu (
    .clk(clk),
    .tw_idx(tw_idx),
    .tw_re(tw_re),
    .tw_im(tw_im)
);

memory_unit mu (
    .clk(clk),
    .nrst(nrst),
    .mem_we(mem_we),
    .mem_addr(mem_addr),
    .mem_wdata_re(mem_wdata_re),
    .mem_wdata_im(mem_wdata_im),
    .mem_rdata_re(mem_rdata_re),
    .mem_rdata_im(mem_rdata_im)
);

butterfly_unit bu (
    .direction(dir_reg),
    .scaling(scaling_reg),
    .x0_re(butterfly_input0_re),
    .x0_im(butterfly_input0_im),
    .x1_re(butterfly_input1_re),
    .x1_im(butterfly_input1_im),
    .twiddle_re(tw_re),
    .twiddle_im(tw_im),
    .y0_re(butterfly_output0_re),
    .y0_im(butterfly_output0_im),
    .y1_re(butterfly_output1_re),
    .y1_im(butterfly_output1_im)
);

endmodule