cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 11)

set(TOP_PROJECT_NAME "OmniSketch")
project(${TOP_PROJECT_NAME})

include_directories(common)
include_directories(sketch)
include_directories(test)

add_executable(driver test/TestSketch.cpp)

add_subdirectory(PcapParser)
