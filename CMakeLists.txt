cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(reaction VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置 include 目录，方便引用头文件
include_directories(${PROJECT_SOURCE_DIR}/include)

# 设置源文件目录，自动获取 src 目录中的所有 .cpp 文件
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# 创建共享库
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# 设置库文件的输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

# 添加 Google Test 子模块（如果使用 Git 克隆 Google Test，或者直接用 vcpkg 安装）
enable_testing()

# 假设你已经安装了 gtest 包，或者使用了 vcpkg
find_package(GTest REQUIRED)

# 自动获取 test 目录中的所有 .cpp 测试文件
file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)

# 创建测试目标
add_executable(runTests ${TEST_SOURCES})

# 链接 Google Test 和你的库
target_link_libraries(runTests GTest::GTest GTest::Main ${PROJECT_NAME})

target_include_directories(runTests PRIVATE ${PROJECT_SOURCE_DIR}/include)

# 添加测试
add_test(NAME reactionTest COMMAND runTests)
