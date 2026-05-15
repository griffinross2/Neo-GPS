# Create a new executable target that will contain all your sources
add_executable(uart_rx_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/uart/uart_rx_tb.sv
    ../../src/uart/uart_rx.sv
    ../../src/uart/uart_tx.sv
)

# Add the Verilated circuit to the target
verilate(uart_rx_tb
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top uart_rx_tb
)