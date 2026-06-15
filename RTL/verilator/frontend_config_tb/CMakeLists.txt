# Create a new executable target that will contain all your sources
add_executable(frontend_config_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/frontend/frontend_config_tb.sv
    ../../src/frontend/frontend_config.sv
)

# Add the Verilated circuit to the target
verilate(frontend_config_tb
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top frontend_config_tb
)