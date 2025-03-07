cmake_minimum_required(VERSION 3.10)

project(reaction VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/include/reaction/*.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

enable_testing()

find_package(GTest REQUIRED)

file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)

add_executable(runTests ${TEST_SOURCES})

target_link_libraries(runTests GTest::GTest GTest::Main ${PROJECT_NAME})

target_include_directories(runTests PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_test(NAME reactionTest COMMAND runTests)
