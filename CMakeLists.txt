cmake_minimum_required(VERSION 3.20)
project(ShowCase)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-O3 -march=native)

# Enable sanitizers for Debug builds
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling sanitizers for Debug build")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)


add_executable(${PROJECT_NAME} src/main.cpp src/mutex_buffer_v1.hpp src/my_unique_ptr.hpp src/vector.hpp src/my_shared_ptr.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)

add_executable(tests
    tests/test_unique_ptr.cpp
    tests/test_shared_ptr.cpp
    tests/test_deque.cpp
    tests/test_vector.cpp
)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
