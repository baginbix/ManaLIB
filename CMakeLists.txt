cmake_minimum_required(VERSION 3.20)
project(ShowCase)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-O3 -march=native)

add_executable(${PROJECT_NAME} src/main.cpp src/mutex_buffer_v1.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
