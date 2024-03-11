cmake_minimum_required(VERSION 3.27)

find_package(unordered_dense CONFIG REQUIRED)

project(Tonic)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_STANDARD 20)

add_executable(Tonic main.cpp
        src/Utils.cpp
        include/Utils.h
        include/hash_table5.hpp
        src/TriangleSampler.cpp
        include/TriangleSampler.h
        include/HeavyEdgesPQ.h)

target_include_directories(Tonic PRIVATE include)

target_link_libraries(Tonic unordered_dense::unordered_dense)