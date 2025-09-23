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

    # Add threading support for thread safety tests
    find_package(Threads REQUIRED)
    target_link_libraries(runTests PRIVATE Threads::Threads)

    # Add compiler flags for thread safety and optimization
    target_compile_options(runTests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-pthread>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O2>
        $<$<CXX_COMPILER_ID:MSVC>:/O2>
    )
    # Discover and register tests
    gtest_discover_tests(runTests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # # Create TSAN build for thread race detection
    # add_executable(runTestsTSAN ${TEST_SOURCES})
    # target_include_directories(runTestsTSAN PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}
    #     ${CMAKE_CURRENT_SOURCE_DIR}/common
    # )
    # target_link_libraries(runTestsTSAN PRIVATE GTest::GTest GTest::Main ${PROJECT_NAME} Threads::Threads)
    # target_compile_options(runTestsTSAN PRIVATE
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=thread>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-g>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-O1>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-omit-frame-pointer>
    # )
    # target_link_options(runTestsTSAN PRIVATE
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=thread>
    # )
    # target_compile_definitions(runTestsTSAN PRIVATE REACTION_FORCE_THREAD_SAFETY=1)

    # # Create UBSAN build for undefined behavior detection
    # add_executable(runTestsUBSAN ${TEST_SOURCES})
    # target_include_directories(runTestsUBSAN PRIVATE
    #     ${CMAKE_CURRENT_SOURCE_DIR}
    #     ${CMAKE_CURRENT_SOURCE_DIR}/common
    # )
    # target_link_libraries(runTestsUBSAN PRIVATE GTest::GTest GTest::Main ${PROJECT_NAME} Threads::Threads)
    # target_compile_options(runTestsUBSAN PRIVATE
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=undefined>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-g>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-O1>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-omit-frame-pointer>
    # )
    # target_link_options(runTestsUBSAN PRIVATE
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=undefined>
    #     $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address>
    # )

else()
    message(WARNING "GTest not found, skipping tests.")
endif()