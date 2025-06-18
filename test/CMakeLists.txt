find_package(GTest)
if(GTest_FOUND)
    enable_testing()
    file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
    add_executable(runTests ${TEST_SOURCES})
    target_link_libraries(runTests PRIVATE GTest::GTest GTest::Main ${PROJECT_NAME})
    gtest_discover_tests(runTests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    message(WARNING "GTest not found, skipping tests.")
endif()