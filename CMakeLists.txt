cmake_minimum_required(VERSION 3.22)
project(ComputerNetwork C)

set(CMAKE_C_STANDARD 11)

include_directories(lib)

add_executable(ComputerNetwork
        lib/MyFunction.h
        lib/datastructure.h
        src/main.c src/MyFunction.c src/MySocket.c src/MySocket.h)

add_executable(test src/test.c)

target_link_libraries(${CMAKE_PROJECT_NAME} ws2_32)
target_link_libraries(test ws2_32)
