cmake_minimum_required(VERSION 3.26)
project(Zadanie_2)

set(CMAKE_CXX_STANDARD 17)

add_executable(Zadanie_2 main.cpp CRC.h)

target_link_libraries(Zadanie_2 Ws2_32.lib)