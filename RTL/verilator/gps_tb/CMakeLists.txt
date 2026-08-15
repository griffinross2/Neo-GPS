# Create a new executable target that will contain all your sources
add_executable(gps_tb
    sim_main.cpp
    spi_host.cpp
    test_program.cpp
)

set(RTL_SOURCES
    ../../testbench/top/gps_tb.sv
    ../../src/spi/spi_device.sv
    ../../src/gps/l1ca_ac_pca_search.sv
    ../../src/gps/l1ca_code.sv
    ../../src/dsp/fft/fft_4096.sv
    ../../src/dsp/fft/butterfly_unit.sv
    ../../src/dsp/fft/memory_unit.sv
    ../../src/dsp/fft/twiddle_unit.sv
    ../../src/dsp/fft/control_unit.sv
    ../../src/memory/spram.v
    ../../src/memory/dpram.v
)

# Add the Verilated circuit to the target
verilate(gps_tb
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top gps_tb
)

target_compile_options(gps_tb PRIVATE -Wno-attributes)