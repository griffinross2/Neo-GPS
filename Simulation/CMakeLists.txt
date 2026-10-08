cmake_minimum_required(VERSION 3.26)
project(NeoGPS)

include(FetchContent)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable
add_library(${PROJECT_NAME} STATIC)

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

# Add source files
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Include directories
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fftw3)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fftw3/libfftw3-3.lib
    COMMAND lib.exe /machine:X64 /def:${CMAKE_CURRENT_SOURCE_DIR}/fftw3/libfftw3-3.def /out:${CMAKE_CURRENT_SOURCE_DIR}/fftw3/libfftw3-3.lib
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fftw3/libfftw3-3.def
    VERBATIM
)
add_custom_target(fftw3_import ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fftw3/libfftw3-3.lib)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fftw3/libfftw3-3.lib)

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    EIGEN_NO_MALLOC
)

# Tests/examples
add_subdirectory(tests/acquisition)
add_subdirectory(tests/track1l1ca)
add_subdirectory(tests/trackl1ca)
add_subdirectory(tests/code)
add_subdirectory(tests/track1e1)
add_subdirectory(tests/tracke1)
add_subdirectory(tests/solve)