// test/test.cpp
#include "gtest/gtest.h"
#include "reaction/dataSource.h"
#include "reaction/expression.h"
#include "reaction/observerNode.h"
#include <iostream>
#include <string>

// 循环依赖问题done
// 重复依赖问题done
// 数据源重绑定done
// 访问权限问题done
// 用户异常提示
// 数据源失效问题
// valueCache功能
// 内存管理问题
// 多线程支持
// corner case检测

TEST(TestConstructor, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb) {
        std::cout << "a = " << aa << ", b = " << bb << std::endl;
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds) {
        static_assert(std::is_same_v<decltype(dsds),std::string>);
        std::cout << "a = " << aa << ", dds = " << dsds << std::endl;
        return std::to_string(aa) + dsds;
    }, a, ds);
    EXPECT_EQ(ds.get(), "13.140000");
    EXPECT_EQ(dds.get(), "113.140000");
}

TEST(TestCommonUse, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb) {
        std::cout << "a = " << aa << ", b = " << bb << std::endl;
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds) {
        static_assert(std::is_same_v<decltype(dsds),std::string>);
        std::cout << "a = " << aa << ", ds = " << dsds << std::endl;
        return std::to_string(aa) + dsds;
    }, a, ds);
    a = 2;
    EXPECT_EQ(ds.get(), "23.140000");
    EXPECT_EQ(dds.get(), "223.140000");
}

TEST(TestConstDataSource, ReactionTest) {
    auto a = reaction::makeConstantDataSource(1);
    auto b = reaction::makeConstantDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb) {
        std::cout << "a = " << aa << ", b = " << bb << std::endl;
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds) {
        static_assert(std::is_same_v<decltype(dsds),std::string>);
        std::cout << "a = " << aa << ", ds = " << dsds << std::endl;
        return std::to_string(aa) + dsds;
    }, a, ds);
}

TEST(TestReset1, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](auto aa, auto bb) {
        std::cout << "a = " << aa << ", b = " << bb << std::endl;
        return std::to_string(aa) + std::to_string(bb);
    }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto bb) {
        static_assert(std::is_same_v<decltype(bb),std::string>);
        std::cout << "a = " << aa << ", ds = " << bb << std::endl;
        return std::to_string(aa) + bb;
    }, a, ds);
    ds.reset([](auto aa, auto bb) {
        std::cout << "after reset, a = " << aa << ", b = " << bb << std::endl;
        return std::to_string(aa) + std::to_string(bb) + "reset";
    }, a, b);
    EXPECT_EQ(ds.get(), "13.140000reset");
}

TEST(TestReset2, ReactionTest) {
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(std::string{"2"});
    auto c = reaction::makeMetaDataSource(std::string{"3"});
    auto d = reaction::makeMetaDataSource(std::string{"4"});
    auto ds = reaction::makeVariableDataSource([](auto aa) {
        return std::to_string(aa);
    }, a);
    auto dds = reaction::makeVariableDataSource([](auto bb) {
        return bb;
    }, b);
    auto ddds = reaction::makeVariableDataSource([](auto cc) {
        return cc;
    }, c);
    EXPECT_EQ(ddds.get(), "3");
    ddds.reset([](auto dd, auto dsds) {
        return dd + dsds + "reset";
    }, d, dds);
    EXPECT_EQ(ddds.get(), "42reset");
    c = "33";
    EXPECT_EQ(ddds.get(), "42reset");
    d = "44";
    EXPECT_EQ(ddds.get(), "442reset");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}