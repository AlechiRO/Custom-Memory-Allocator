add_library(allocator_logic alloc.c)

target_include_directories(allocator_logic PRIVATE 
    ${CMAKE_SOURCE_DIR}/inc
)