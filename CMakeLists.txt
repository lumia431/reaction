cmake_minimum_required(VERSION 3.10)

project(reaction VERSION 1.0)

# 强制设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-std=c++20)  # 强制使用 C++20

if(MSVC)
    add_compile_options(/GR-)
else()
    add_compile_options(-fno-rtti)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/include/reaction/*.h)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

file(GLOB EXAMPLE_SOURCES ${PROJECT_SOURCE_DIR}/example/*.cpp)
foreach(example_file ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_file} NAME_WE)
    add_executable(${example_name} ${example_file})
    target_link_libraries(${example_name} ${PROJECT_NAME})
    target_include_directories(${example_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
endforeach()

enable_testing()

find_package(GTest REQUIRED)

file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)

add_executable(runTests ${TEST_SOURCES})

target_link_libraries(runTests GTest::GTest GTest::Main ${PROJECT_NAME})

target_include_directories(runTests PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_test(NAME reactionTest COMMAND runTests)

file(GLOB_RECURSE CFG_FILES src/* CMakeLists.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CFG_FILES})
