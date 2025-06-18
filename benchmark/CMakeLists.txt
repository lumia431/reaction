find_package(benchmark)
# find_package(rxcpp)

if(benchmark_FOUND)
    file(GLOB BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

    foreach(bench_file ${BENCH_SOURCES})
        get_filename_component(bench_name ${bench_file} NAME_WE)
        add_executable(${bench_name} ${bench_file})
        target_link_libraries(${bench_name} PRIVATE benchmark::benchmark ${PROJECT_NAME} pthread)
    endforeach()
else()
    message(WARNING "Google Benchmark not found, skipping benchmarks.")
endif()
