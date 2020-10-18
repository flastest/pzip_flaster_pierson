cmake_minimum_required(VERSION 3.17)
project(pzip_flaster_pierson)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -ggdb3 -Og -DDEBUG -pthread")

add_executable(pzip pzip.cc)
