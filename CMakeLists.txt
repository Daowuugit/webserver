cmake_minimum_required(VERSION 3.10)

project(server)

add_executable(server main.cpp ThreadPool.h http.h http.cpp)
add_executable(client client.cpp)

TARGET_LINK_LIBRARIES( server
    pthread
)