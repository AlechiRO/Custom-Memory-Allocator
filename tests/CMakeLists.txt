find_library(CUNIT_LIBRARIES NAMES cunit)
find_path(CUNIT_INCLUDE_DIRS NAMES CUnit/CUnit.h)


if(CUNIT_LIBRARIES AND CUNIT_INCLUDE_DIRS)

    # Stack test executable
    add_executable (ALLOC_TEST_DRIVER 
        test_alloc.c
    )
    target_link_libraries(ALLOC_TEST_DRIVER PRIVATE 
        ${CUNIT_LIBRARIES}
        $<TARGET_OBJECTS:allocator_logic>
    )
    target_include_directories(ALLOC_TEST_DRIVER PRIVATE 
        ${CUNIT_INCLUDE_DIRS} 
        ${CMAKE_SOURCE_DIR}/inc
    )


    # Adding Tests
    add_test(NAME test_alloc COMMAND ALLOC_TEST_DRIVER)

else()
    message(WARNING "CUnit not found!")
endif()