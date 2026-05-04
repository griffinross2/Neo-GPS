# Create a new executable target that will contain all your sources
add_executable(spi_tx sim_main.cpp)

set(RTL_SOURCES
    ../../src/spi_tx.sv
    ../../testbench/spi_tx_tb.sv
)

# Add the Verilated circuit to the target
verilate(spi_tx
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top spi_tx_tb
)