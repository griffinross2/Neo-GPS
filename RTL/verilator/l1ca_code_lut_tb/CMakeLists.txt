# Create a new executable target that will contain all your sources
add_executable(l1ca_code_lut_tb sim_main.cpp)

set(RTL_SOURCES
    ../../testbench/gps/l1ca_code_lut_tb.sv
    ../../src/gps/l1ca_code_lut.sv
)

# Add the Verilated circuit to the target
verilate(l1ca_code_lut_tb
    INCLUDE_DIRS ../../inc
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --trace-fst --threads 1 --top l1ca_code_lut_tb
)

# put code rom in output
add_custom_command(TARGET l1ca_code_lut_tb POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/gps/l1ca_code_rom.hex"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../l1ca_code_rom.hex"
    COMMENT "Copying ROM to outputs directory"
)