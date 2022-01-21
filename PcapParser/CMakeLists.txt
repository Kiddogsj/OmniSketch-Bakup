cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 11)

project(PcapParser)

set (PARSER_PROJECT_NAME "PcapParser")
project (${PARSER_PROJECT_NAME})

include_directories(${TOP_PROJECT_NAME}_SOURCE_DIR/common)

add_executable(parser parser.cpp)