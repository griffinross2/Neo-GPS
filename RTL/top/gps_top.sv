`timescale 1ns/1ns

module gps_top (
    input CLK, ck_rst,
    output LED [0:3],
    input BTN[0:3],
    input ja_0,
    output ja_2,
    inout ja_1, ja_3, ja_4, ja_6, ja_7,
    input jb_0, jb_1, jb_2, jb_3, jb_6,
    output jb_4, jb_5, jb_7,
    input ck_io3, ck_io4, ck_io5, 
    output ck_io6
);

    wire nrst;
    wire core_clk;
    wire mmcm_locked;

    // Assert reset until core_clk is ready
    assign nrst = ck_rst & mmcm_locked;

    wire i0, i1, q0, q1;
    wire gnss_clk;
    wire frontend_cs, frontend_dout, frontend_sck;

    assign gnss_clk = jb_6;
    assign i0 = jb_2;
    assign i1 = jb_3;
    assign q0 = jb_0;
    assign q1 = jb_1;
    assign jb_4 = frontend_dout;
    assign jb_5 = frontend_sck;
    assign jb_7 = frontend_cs;
    // assign {sck, dout, cs} = 3'b100;

    wire mcu_sck, mcu_cs, mcu_dout, mcu_din;
    assign mcu_sck = ck_io3;
    assign mcu_cs = ck_io4;
    assign mcu_din = ck_io5;
    assign ck_io6 = mcu_dout;

    logic config_start;
    logic config_busy;
    
    wire clkfb_out, clkfb_in;
    wire core_clk_unbuf;

    MMCME2_BASE #(
        .BANDWIDTH("OPTIMIZED"),
        .CLKIN1_PERIOD(10.000),
        .DIVCLK_DIVIDE(1),
        .CLKFBOUT_MULT_F(10.000),
        .CLKFBOUT_PHASE(0.000),
        .CLKOUT0_DIVIDE_F(20.000),
        .CLKOUT0_PHASE(0.000),
        .CLKOUT0_DUTY_CYCLE(0.500),
        .REF_JITTER1(0.010),
        .STARTUP_WAIT("FALSE")
    ) mmcm_inst (
        .CLKIN1(CLK),
        .CLKFBIN(clkfb_in),
        .CLKFBOUT(clkfb_out),
        .CLKFBOUTB(),
        .CLKOUT0(core_clk_unbuf),
        .CLKOUT0B(),
        .CLKOUT1(),
        .CLKOUT1B(),
        .CLKOUT2(),
        .CLKOUT2B(),
        .CLKOUT3(),
        .CLKOUT3B(),
        .CLKOUT4(),
        .CLKOUT5(),
        .CLKOUT6(),
        .LOCKED(mmcm_locked),
        .PWRDWN(1'b0),
        .RST(~ck_rst)
    );

    BUFG clkfb_bufg (.I(clkfb_out), .O(clkfb_in));
    BUFG core_clk_bufg (.I(core_clk_unbuf), .O(core_clk));

    always_ff @(posedge core_clk, negedge nrst) begin
        if (~nrst) begin
            config_start <= 1'b1;
        end else begin
            if (config_busy) begin
                config_start <= 1'b0;
            end
        end
    end

    assign LED[3] = config_busy;

    (* keep_hierarchy = "yes" *)
    frontend_config #(
        .SCLK_DIV(8'd50)
    ) frontend_config_inst (
        .clk(core_clk),
        .nrst(nrst),
        .config_start(config_start),
        .config_busy(config_busy),
        .sclk(frontend_sck),
        .cs(frontend_cs),
        .sdata(frontend_dout)
    );

    (* keep_hierarchy = "yes" *)
    gps gps_inst (
        .gps_clk(gnss_clk),
        .core_clk(core_clk),
        .nrst(nrst),
        .signal_in(i1),
        .sck(mcu_sck),
        .sdi(mcu_din),
        .sdo(mcu_dout),
        .cs(mcu_cs)
    );

endmodule