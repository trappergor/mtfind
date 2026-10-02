cmake_minimum_required(VERSION 3.16)
project(mtfind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(mtfind src/main.cpp)

enable_testing()
add_subdirectory(tests)