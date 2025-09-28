# Benchmark CMakeLists.txt for Reaction Framework Cache Performance Tests
cmake_minimum_required(VERSION 3.10)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable threading
find_package(Threads REQUIRED)

# Find required packages
find_package(benchmark)
find_package(rxcpp)

if(benchmark_FOUND)
    add_executable(bench_cache bench_cache.cpp)
    target_include_directories(bench_cache PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(bench_cache PRIVATE ${PROJECT_NAME})

    add_executable(bench_multi_thread bench_multi_thread.cpp)
    target_include_directories(bench_multi_thread PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(bench_multi_thread PRIVATE benchmark::benchmark ${PROJECT_NAME} pthread)
    message(STATUS "Building multi-thread performance benchmarks")

    if(rxcpp_FOUND)
        add_executable(bench_comparison bench_comparison.cpp)
        target_include_directories(bench_comparison PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
        target_link_libraries(bench_comparison PRIVATE benchmark::benchmark ${PROJECT_NAME} pthread)
        message(STATUS "Building framework comparison benchmarks")
    else()
        message(WARNING "RxCpp not found, skipping comparison benchmarks")
    endif()

else()
    message(WARNING "Google Benchmark not found, skipping original benchmarks.")
endif()
