cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 11)

project(SketchLab_CPP)

include_directories(common)
include_directories(sketch)
include_directories(test)

add_executable(driver test/TestSketch.cpp)
