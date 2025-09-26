# Benchmark CMakeLists.txt for Reaction Framework Cache Performance Tests
cmake_minimum_required(VERSION 3.15)

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

    if(rxcpp_FOUND)
        add_executable(bench_comparison bench_comparison.cpp)
        target_include_directories(bench_comparison PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
        target_link_libraries(bench_comparison PRIVATE benchmark::benchmark ${PROJECT_NAME})
        message(STATUS "Building framework comparison benchmarks")
    else()
        message(WARNING "RxCpp not found, skipping comparison benchmarks")
    endif()

else()
    message(WARNING "Google Benchmark not found, skipping original benchmarks.")
endif()
