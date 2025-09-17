find_package(GTest)
if(GTest_FOUND)
    enable_testing()

    # Collect all test sources from unit directory
    file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp)

    # Create the test executable
    add_executable(runTests ${TEST_SOURCES})

    # Set include directories for tests
    target_include_directories(runTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )

    # Link libraries
    target_link_libraries(runTests PRIVATE GTest::GTest GTest::Main ${PROJECT_NAME})

    # Discover and register tests
    gtest_discover_tests(runTests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

else()
    message(WARNING "GTest not found, skipping tests.")
endif()