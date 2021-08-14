cmake_minimum_required (VERSION 3.0.0)
set (CMAKE_CXX_STANDARD 17)

project(test)
add_executable(test)
target_sources(test PRIVATE test.cc)
