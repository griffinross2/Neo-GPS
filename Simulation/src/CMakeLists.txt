cmake_minimum_required(VERSION 3.26)

target_sources(${PROJECT_NAME} PRIVATE
    l1ca_search_ac_pca.cpp
    l1ca_code.cpp
    l1ca_channel.cpp
    filters.cpp
    e1_code.cpp
    e1_search_ac_pca.cpp
    e1_channel.cpp
    b1c_code.cpp
    b1c_search_ac_pca.cpp
    l1c_code.cpp
    l1c_search_ac_pca.cpp
    l1ca_ephemeris.cpp
    tools.cpp
    e1_ephemeris.cpp
)