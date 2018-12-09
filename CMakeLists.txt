cmake_minimum_required(VERSION 3.5)
project(Test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp ikcp.c)
add_executable(Test ${SOURCE_FILES})