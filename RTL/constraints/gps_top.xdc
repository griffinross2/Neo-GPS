set_false_path -from [get_clocks core_clk] -to [get_clocks gnss_clk_pin]
set_false_path -from [get_clocks gnss_clk_pin] -to [get_clocks core_clk]

set_false_path -from [get_clocks core_clk] -to [get_clocks mcu_sck]
set_false_path -from [get_clocks mcu_sck] -to [get_clocks core_clk]
