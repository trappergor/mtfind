cmake_minimum_required(VERSION 3.16)
project(mtfind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(mtfind_lib
    src/args.cpp
    src/searcher.cpp
)

target_include_directories(mtfind_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(mtfind src/main.cpp)
target_link_libraries(mtfind PRIVATE mtfind_lib)

enable_testing()
add_subdirectory(tests)