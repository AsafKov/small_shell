cmake_minimum_required(VERSION 3.22)
project(ex1)

set(CMAKE_CXX_STANDARD 11)

add_executable(ex1 smash.cpp Commands.cpp signals.cpp)
