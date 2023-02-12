cmake_minimum_required(VERSION 3.10)

project(server)

add_executable(server main.cpp http.h)
add_executable(client client.cpp)