cmake_minimum_required(VERSION 3.26)

set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2)

include(pico-sdk/pico_sdk_init.cmake)

project(NeoGPS_RP2350_Firmware)

pico_sdk_init()

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable
add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib)
target_link_libraries(${PROJECT_NAME} PRIVATE hardware_pio)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

pico_add_extra_outputs(${PROJECT_NAME})

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/pio/parallel_interface.pio)