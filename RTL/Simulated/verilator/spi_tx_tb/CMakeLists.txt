# Create a new executable target that will contain all your sources
add_executable(spi_tx_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/spi/spi_tx_tb.sv
    ../../src/spi/spi_tx.sv
)

# Add the Verilated circuit to the target
verilate(spi_tx_tb
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top spi_tx_tb
)