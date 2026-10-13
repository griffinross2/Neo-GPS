`timescale 1ns/1ns

module gps_top (
    input CLK, ck_rst,
    output LED [0:3],
    input BTN[0:3],
    input jb_0, jb_1, jb_2, jb_3, jb_6,
    output jb_4, jb_5, jb_7,
    input ck_io3, ck_io4, 
    inout ck_io26, ck_io27, ck_io28, ck_io29, ck_io30, ck_io31, ck_io32, ck_io33, ck_io34, ck_io35, ck_io36, ck_io37, ck_io38, ck_io39, ck_io40, ck_io41
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

    wire mcu_clk, mcu_cs;
    wire [15:0] mcu_dat;
    assign mcu_clk = ck_io3;
    assign mcu_cs = ck_io4;
    tran t0 (ck_io26, mcu_dat[0]);
    tran t1 (ck_io27, mcu_dat[1]);
    tran t2 (ck_io28, mcu_dat[2]);
    tran t3 (ck_io29, mcu_dat[3]);
    tran t4 (ck_io30, mcu_dat[4]);
    tran t5 (ck_io31, mcu_dat[5]);
    tran t6 (ck_io32, mcu_dat[6]);
    tran t7 (ck_io33, mcu_dat[7]);
    tran t8 (ck_io34, mcu_dat[8]);
    tran t9 (ck_io35, mcu_dat[9]);
    tran t10 (ck_io36, mcu_dat[10]);
    tran t11 (ck_io37, mcu_dat[11]);
    tran t12 (ck_io38, mcu_dat[12]);
    tran t13 (ck_io39, mcu_dat[13]);
    tran t14 (ck_io40, mcu_dat[14]);
    tran t15 (ck_io41, mcu_dat[15]);

    logic config_start;
    logic config_busy;
    
    wire clkfb_out, clkfb_in;
    wire core_clk_unbuf;

    // core_clk = 50 MHz, input CLK = 100 MHz
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
        .if_clk(mcu_clk),
        .if_cs(mcu_cs),
        .if_dat(mcu_dat)
    );

endmodule