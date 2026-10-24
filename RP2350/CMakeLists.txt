cmake_minimum_required(VERSION 3.26)

include(FetchContent)

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

# Eigen
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 5.0.1
)

FetchContent_MakeAvailable(eigen)

FetchContent_GetProperties(eigen
    POPULATED eigen_POPULATED
    SOURCE_DIR eigen_SOURCE_DIR
    BINARY_DIR eigen_BINARY_DIR
)

if(eigen_POPULATED)
    message(STATUS "Eigen has been populated.")
    message(STATUS "Source directory: ${eigen_SOURCE_DIR}")
    message(STATUS "Binary directory: ${eigen_BINARY_DIR}")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib)
target_link_libraries(${PROJECT_NAME} PRIVATE hardware_pio)
target_link_libraries(${PROJECT_NAME} PRIVATE hardware_dma)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

pico_add_extra_outputs(${PROJECT_NAME})

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/pio/parallel_interface.pio)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_DEFAULT_UART=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_DEFAULT_UART_TX_PIN=20)
target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_DEFAULT_UART_RX_PIN=21)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    EIGEN_NO_MALLOC
)

string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")