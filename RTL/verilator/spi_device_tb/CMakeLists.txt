# Create a new executable target that will contain all your sources
add_executable(spi_device_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/spi/spi_device_tb.sv
    ../../src/spi/spi_device.sv
)

# Add the Verilated circuit to the target
verilate(spi_device_tb
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top spi_device_tb
)