cmake_minimum_required(VERSION 3.10)

project(reaction VERSION 1.0 LANGUAGES CXX)

option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)
option(BUILD_EXAMPLES "Build example projects" OFF)
option(BUILD_TESTS "Build test projects" OFF)
option(BUILD_BENCHMARKS "Build benchmark projects" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy-19;
    -quiet;
    -checks=modernize-*,readability-*,performance-*,-readability-identifier-length,-readability-magic-numbers,-modernize-use-trailing-return-type,-readability-function-cognitive-complexity
  )
endif()

if(MSVC)
    add_compile_options(/W4 /GR-)
else()
    add_compile_options(-Wall -Wextra -pedantic -std=c++2a)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

file(GLOB HEADERS_LIST "${CMAKE_CURRENT_SOURCE_DIR}/include/reaction/*.h")
foreach(header_file ${HEADERS_LIST})
    target_sources(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${header_file}>
    )
endforeach()
target_sources(${PROJECT_NAME} INTERFACE ${HEADERS})

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

include(GNUInstallDirs)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(WIN32)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path prefix" FORCE)
        string(REPLACE "\\" "/" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    else()
        set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install path prefix" FORCE)
    endif()
endif()

install(
    DIRECTORY include/reaction
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    EXPORT ${PROJECT_NAME}Targets
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)