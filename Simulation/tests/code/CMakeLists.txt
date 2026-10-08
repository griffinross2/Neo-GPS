cmake_minimum_required(VERSION 3.26)

project(Code)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../inc)

target_link_libraries(${PROJECT_NAME} PRIVATE NeoGPS)

# Move FFTW DLL into build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/../../fftw3/libfftw3-3.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/libfftw3-3.dll)