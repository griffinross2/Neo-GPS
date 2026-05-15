# Create a new executable target that will contain all your sources
add_executable(sd_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/sd/sd_tb.sv
    ../../src/memory/simple_dual_two_clocks.v
    ../../src/sd/sd_cmd.sv
    ../../src/sd/sd_data.sv
    ../../src/sd/sd_crc.sv
    ../../src/sd/sd_host.sv
    ../../src/sd/sd_fake/common.v
    ../../src/sd/sd_fake/sd_brams.v
    ../../src/sd/sd_fake/sd_link.v
    ../../src/sd/sd_fake/sd_mgr.v
    ../../src/sd/sd_fake/sd_phy.v
    ../../src/sd/sd_fake/sd_top.v
    ../../src/sd/sd_fake/sd_wishbone.v
)

# Add the Verilated circuit to the target
verilate(sd_tb
    INCLUDE_DIRS ../../src/sd/sd_fake
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top sd_tb
)