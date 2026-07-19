# Create a new executable target that will contain all your sources
add_executable(fft_4096_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/dsp/fft/fft_4096_tb.sv
    ../../src/dsp/fft/fft_4096.sv
    ../../src/dsp/fft/butterfly_unit.sv
    ../../src/dsp/fft/memory_unit.sv
    ../../src/dsp/fft/twiddle_unit.sv
    ../../src/dsp/fft/control_unit.sv
    ../../src/memory/spram.v
)

# Add the Verilated circuit to the target
verilate(fft_4096_tb
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top fft_4096_tb
)

# put twiddle_rom in output
add_custom_command(TARGET fft_4096_tb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/dsp/fft/twiddle_rom.hex"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../twiddle_rom.hex"
    COMMENT "Copying ROM to outputs directory"
)