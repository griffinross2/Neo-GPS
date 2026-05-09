# Create a new executable target that will contain all your sources
add_executable(l1ca_code_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/gps/l1ca_code_tb.sv
    ../../src/gps/l1ca_code.sv
)

# Add the Verilated circuit to the target
verilate(l1ca_code_tb
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top l1ca_code_tb
)