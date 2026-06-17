cmake_minimum_required(VERSION 3.29)
project(main)

set(CMAKE_CXX_STANDARD 20)

# Point to the file inside the sub-folder
add_executable(my_server main.cpp)