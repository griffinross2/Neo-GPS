cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0074 NEW)
project(uart_rx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})

if(NOT verilator_FOUND)
    message(
        FATAL_ERROR
        "Verilator was not found. Either install it, or set the VERILATOR_ROOT environment variable"
    )
endif()

# Create a new executable target that will contain all your sources
add_executable(uart_tx sim_main.cpp)

set(RTL_SOURCES ../../src/uart_tx.sv
    ../../testbench/uart_tx_tb.sv
)

# Add the Verilated circuit to the target
verilate(uart_tx
    INCLUDE_DIR .
    SOURCES ${RTL_SOURCES}
    VERILATOR_ARGS --timing --top uart_tx_tb
)
