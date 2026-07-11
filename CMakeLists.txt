cmake_minimum_required(VERSION 3.10) 
project(MemAllocator VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Provide coverage flags for compilation
# --coverage is a shortuct for -fprofile-arcs -ftest-coverage 
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)