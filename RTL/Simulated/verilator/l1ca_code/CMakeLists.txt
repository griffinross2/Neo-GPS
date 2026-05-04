# Create a new executable target that will contain all your sources
add_executable(l1ca_code sim_main.cpp)

set(RTL_SOURCES
    ../../src/l1ca_code.sv
    ../../testbench/l1ca_code_tb.sv
)

# Add the Verilated circuit to the target
verilate(l1ca_code
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top l1ca_code_tb
)