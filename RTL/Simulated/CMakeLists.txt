cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0074 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(testbenches)

find_package(verilator HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_ROOT})

if(NOT verilator_FOUND)
    message(
        FATAL_ERROR
        "Verilator was not found. Either install it, or set the VERILATOR_ROOT environment variable"
    )
endif()

add_subdirectory(verilator/spi_tx_tb)
add_subdirectory(verilator/uart_tx_tb)
add_subdirectory(verilator/uart_rx_tb)
add_subdirectory(verilator/l1ca_code_tb)
add_subdirectory(verilator/sd_tb)