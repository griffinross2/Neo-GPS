# Create a new executable target that will contain all your sources
add_executable(uart_tx sim_main.cpp)

set(RTL_SOURCES
    ../../src/uart_tx.sv
    ../../testbench/uart_tx_tb.sv
)

# Add the Verilated circuit to the target
verilate(uart_tx
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top uart_tx_tb
)