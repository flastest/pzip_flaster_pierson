cmake_minimum_required(VERSION 3.17)
project(pzip C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -ggdb3 -Og -DDEBUG -pthread")

add_executable(pzip wzip-talib.c)