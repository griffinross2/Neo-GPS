# Create a new executable target that will contain all your sources
add_executable(uart_rx sim_main.cpp)

set(RTL_SOURCES
    ../../src/uart_rx.sv
    ../../src/uart_tx.sv
    ../../testbench/uart_rx_tb.sv
)

# Add the Verilated circuit to the target
verilate(uart_rx
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top uart_rx_tb
)