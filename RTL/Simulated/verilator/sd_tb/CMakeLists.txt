# Create a new executable target that will contain all your sources
add_executable(sd_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/sd_tb.sv
    ../../src/sd_cmd.sv
)

# Add the Verilated circuit to the target
verilate(sd_tb
    INCLUDE_DIRS .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top sd_tb
)