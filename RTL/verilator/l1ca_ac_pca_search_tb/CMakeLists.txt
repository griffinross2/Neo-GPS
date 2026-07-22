# Create a new executable target that will contain all your sources
add_executable(l1ca_ac_pca_search_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/gps/l1ca_ac_pca_search_tb.sv
    ../../src/gps/l1ca_ac_pca_search.sv
    ../../src/gps/l1ca_code.sv
    ../../src/dsp/fft/fft_4096.sv
    ../../src/dsp/fft/butterfly_unit.sv
    ../../src/dsp/fft/memory_unit.sv
    ../../src/dsp/fft/twiddle_unit.sv
    ../../src/dsp/fft/control_unit.sv
    ../../src/memory/spram.v
)

# Add the Verilated circuit to the target
verilate(l1ca_ac_pca_search_tb
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top l1ca_ac_pca_search_tb
)