# Create a new executable target that will contain all your sources
add_executable(parallel_interface_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/parallel_interface/parallel_interface_tb.sv
    ../../src/parallel_interface/parallel_interface.sv
)

# Add the Verilated circuit to the target
verilate(parallel_interface_tb
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top parallel_interface_tb
)