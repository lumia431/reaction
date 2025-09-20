find_package(benchmark)
find_package(rxcpp)

if(benchmark_FOUND)
    # 核心性能基准测试（默认构建）
    add_executable(bench_multi_thread bench_multi_thread.cpp)
    target_include_directories(bench_multi_thread PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(bench_multi_thread PRIVATE benchmark::benchmark ${PROJECT_NAME} pthread)
    message(STATUS "Building multi-thread performance benchmarks")

    # 框架对比基准测试（自动检测RxCpp）
    if(rxcpp_FOUND)
        add_executable(bench_comparison bench_comparison.cpp)
        target_include_directories(bench_comparison PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
        target_link_libraries(bench_comparison PRIVATE benchmark::benchmark ${PROJECT_NAME} pthread)
        message(STATUS "Building framework comparison benchmarks")
    else()
        message(WARNING "RxCpp not found, skipping comparison benchmarks")
    endif()

else()
    message(WARNING "Google Benchmark not found, skipping all benchmarks.")
endif()
