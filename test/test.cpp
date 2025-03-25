// test/test.cpp
#include "gtest/gtest.h"
#include "reaction/dataSource.h"
#include <vector>
#include <iostream>
#include <numeric>

TEST(TestConstructor, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    EXPECT_EQ(a->get(), 1);
    EXPECT_EQ(b->get(), 3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);
    EXPECT_EQ(ds->get(), "13.140000");
    EXPECT_EQ(dds->get(), "113.140000");
}

TEST(TestCommonUse, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);
    *a = 2;
    EXPECT_EQ(ds->get(), "23.140000");
    EXPECT_EQ(dds->get(), "223.140000");
}

TEST(TestComplexCal, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    a->setName("a");

    auto dsA = reaction::makeVariableDataSource([](int aa)
                                                { return aa; }, a);
    dsA->setName("dsA");

    auto dsB = reaction::makeVariableDataSource([](int aa, int dsAValue)
                                                { return aa + dsAValue; }, a, dsA);
    dsB->setName("dsB");

    auto dsC = reaction::makeVariableDataSource([](int aa, int dsAValue, int dsBValue)
                                                { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
    dsC->setName("dsC");

    auto dsD = reaction::makeVariableDataSource([](int dsAValue, int dsBValue, int dsCValue)
                                                { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
    dsD->setName("dsD");

    auto dsE = reaction::makeVariableDataSource([](int dsBValue, int dsCValue, int dsDValue)
                                                { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    dsE->setName("dsE");

    EXPECT_EQ(dsA->get(), 1);
    EXPECT_EQ(dsB->get(), 2);
    EXPECT_EQ(dsC->get(), 4);
    EXPECT_EQ(dsD->get(), 7);
    EXPECT_EQ(dsE->get(), 15);

    *a = 10;
    EXPECT_EQ(dsA->get(), 10);
    EXPECT_EQ(dsB->get(), 20);
    EXPECT_EQ(dsC->get(), 40);
    EXPECT_EQ(dsD->get(), 70);
    EXPECT_EQ(dsE->get(), 870);
}

TEST(TestConstDataSource, ReactionTest)
{
    auto a = reaction::makeConstantDataSource(1);
    auto b = reaction::makeConstantDataSource(3.14);
    // *a = 2;
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);
}

TEST(TestAction, ActionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](auto aa, auto bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    int val = 10;
    auto dds = reaction::makeActionSource([&val](auto aa, auto dsds)
                                          { val = aa; }, a, ds);
    EXPECT_EQ(val, 1);
    *a = 2;
    EXPECT_EQ(val, 2);
}

TEST(TestReset, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(std::string{"2"});
    auto c = reaction::makeMetaDataSource(std::string{"3"});
    auto d = reaction::makeMetaDataSource(std::string{"4"});
    auto ds = reaction::makeVariableDataSource([](auto aa)
                                               { return std::to_string(aa); }, a);
    auto dds = reaction::makeVariableDataSource([](auto bb)
                                                { return bb; }, b);
    auto ddds = reaction::makeVariableDataSource([](auto cc)
                                                 { return cc; }, c);
    EXPECT_EQ(ddds->get(), "3");
    ddds->set([](auto dd, auto dsds)
              { return dd + dsds + "set"; }, d, dds);
    EXPECT_EQ(ddds->get(), "42set");
    *c = "33";
    EXPECT_EQ(ddds->get(), "42set");
    *d = "44";
    EXPECT_EQ(ddds->get(), "442set");
}

TEST(TestSelfDependency, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(2);
    auto c = reaction::makeMetaDataSource(3);

    auto dsA = reaction::makeVariableDataSource([](int aa)
                                                { return aa; }, a);

    EXPECT_EQ(dsA->set([](int aa, int dsAValue)
                       { return aa + dsAValue; }, a, dsA),
              false);
}

TEST(TestRepeatDependency, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(2);
    auto c = reaction::makeMetaDataSource(3);

    int triggerCount = 0;
    auto dsA = reaction::makeVariableDataSource([&triggerCount](int aa, int bb)
                                                {
                                                    ++triggerCount;
                                                    return aa + bb; }, a, b);

    auto dsB = reaction::makeVariableDataSource([](int cc, int dsAVal)
                                                { return cc + dsAVal; }, c, dsA);

    triggerCount = 0;
    *a = 2;
    EXPECT_EQ(triggerCount, 1);

    dsB->set([](int aa, int dsAVal)
             { return aa + dsAVal; }, a, dsA);

    triggerCount = 0;
    *a = 3;
    EXPECT_EQ(triggerCount, 3);
}

TEST(TestCycleDependency, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(2);
    auto c = reaction::makeMetaDataSource(3);

    auto dsA = reaction::makeVariableDataSource([](int bb)
                                                { return bb; }, b);

    auto dsB = reaction::makeVariableDataSource([](int cc)
                                                { return cc; }, c);

    auto dsC = reaction::makeVariableDataSource([](int aa)
                                                { return aa; }, a);

    a->setName("a");
    b->setName("b");
    c->setName("c");
    dsA->setName("dsA");
    dsB->setName("dsB");
    dsC->setName("dsC");

    dsA->set([](int bb, int dsBValue)
             { return bb + dsBValue; }, b, dsB);

    dsB->set([](int cc, int dsCValue)
             { return cc * dsCValue; }, c, dsC);

    EXPECT_EQ(dsC->set([](int aa, int dsAValue)
                       { return aa - dsAValue; }, a, dsA),
              false);
}

TEST(TestCopy, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);

    auto dds_copy = dds;
    EXPECT_EQ(dds_copy->get(), "113.140000");
    EXPECT_EQ(dds->get(), "113.140000");
    *a = 2;
    EXPECT_EQ(dds_copy->get(), "223.140000");
    EXPECT_EQ(dds->get(), "223.140000");
}

TEST(TestMove, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto ds = reaction::makeVariableDataSource([](int aa, double bb)
                                               { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource([](auto aa, auto dsds)
                                                { return std::to_string(aa) + dsds; }, a, ds);

    auto dds_copy = std::move(dds);
    EXPECT_EQ(dds_copy->get(), "113.140000");
    EXPECT_FALSE(static_cast<bool>(dds));
    EXPECT_THROW(dds->get(), std::runtime_error);
    *a = 2;
    EXPECT_EQ(dds_copy->get(), "223.140000");
    EXPECT_FALSE(static_cast<bool>(dds));
}

TEST(TestValueChangeTrigger, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(3.14);
    auto c = reaction::makeMetaDataSource("cc");
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds = reaction::makeVariableDataSource([&triggerCountA](int aa, double bb)
                                               {
                                                ++triggerCountA;
                                                return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::makeVariableDataSource<reaction::ValueChangeTrigger>([&triggerCountB](auto cc, auto dsds)
                                                                              {
                                                                               ++triggerCountB;
                                                                               return cc + dsds; }, c, ds);
    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(triggerCountB, 1);
    *a = 1;
    EXPECT_EQ(triggerCountA, 2);
    EXPECT_EQ(triggerCountB, 1);

    *a = 2;
    EXPECT_EQ(triggerCountA, 3);
    EXPECT_EQ(triggerCountB, 2);
}

TEST(TestThresholdTrigger, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    auto b = reaction::makeMetaDataSource(2);
    auto c = reaction::makeMetaDataSource(3);
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds = reaction::makeVariableDataSource([&triggerCountA](int aa, double bb)
                                               {
                                                ++triggerCountA;
                                                return aa + bb; }, a, b);
    auto dds = reaction::makeVariableDataSource<reaction::ThresholdTrigger>([&triggerCountB](auto cc, auto dsds)
                                                                            {
                                                                             ++triggerCountB;
                                                                             return cc + dsds; }, c, ds);
    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(triggerCountB, 1);
    *a = 2;
    EXPECT_EQ(triggerCountA, 2);
    EXPECT_EQ(triggerCountB, 2);
    EXPECT_EQ(ds->get(), 4);
    EXPECT_EQ(dds->get(), 7);

    dds->setThreshold([](auto cc, auto dsds)
                      { return cc + dsds < 10; }, c, ds);
    *a = 5;
    EXPECT_EQ(triggerCountA, 3);
    EXPECT_EQ(triggerCountB, 2);
    EXPECT_EQ(dds->get(), 7);
}

TEST(TestClose, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    a->setName("a");

    auto b = reaction::makeMetaDataSource(2);
    b->setName("b");

    auto dsA = reaction::makeVariableDataSource([](int aa)
                                                { return aa; }, a);
    dsA->setName("dsA");

    auto dsB = reaction::makeVariableDataSource([](int aa, int dsAValue)
                                                { return aa + dsAValue; }, a, dsA);
    dsB->setName("dsB");

    auto dsC = reaction::makeVariableDataSource([](int aa, int dsAValue, int dsBValue)
                                                { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
    dsC->setName("dsC");

    auto dsD = reaction::makeVariableDataSource([](int dsAValue, int dsBValue, int dsCValue)
                                                { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
    dsD->setName("dsD");

    auto dsE = reaction::makeVariableDataSource([](int dsBValue, int dsCValue, int dsDValue)
                                                { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    dsE->setName("dsE");

    auto dsF = reaction::makeVariableDataSource([](int aa, int bb)
                                                { return aa + bb; }, a, b);
    dsF->setName("dsF");

    auto dsG = reaction::makeVariableDataSource([](int dsAValue, int dsFValue)
                                                { return dsAValue + dsFValue; }, dsA, dsF);
    dsG->setName("dsG");

    dsA->close();
    EXPECT_FALSE(static_cast<bool>(dsA));
    EXPECT_FALSE(static_cast<bool>(dsB));
    EXPECT_FALSE(static_cast<bool>(dsC));
    EXPECT_FALSE(static_cast<bool>(dsD));
    EXPECT_FALSE(static_cast<bool>(dsE));
    EXPECT_TRUE(static_cast<bool>(dsF));
    EXPECT_FALSE(static_cast<bool>(dsG));
}

TEST(TestDirectFailureStrategy, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    a->setName("a");

    auto b = reaction::makeMetaDataSource(2);
    b->setName("b");

    auto dsB = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsC = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsD = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsE = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsF = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsG = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    dsB->setName("dsB");
    dsC->setName("dsC");
    dsD->setName("dsD");
    dsE->setName("dsE");
    dsF->setName("dsF");
    dsG->setName("dsG");

    {
        auto dsA = reaction::makeVariableDataSource([](int aa)
                                                    { return aa; }, a);
        dsA->setName("dsA");

        dsB->set([](int aa, int dsAValue)
                 { return aa + dsAValue; }, a, dsA);

        dsC->set([](int aa, int dsAValue, int dsBValue)
                 { return aa + dsAValue + dsBValue; }, a, dsA, dsB);

        dsD->set([](int dsAValue, int dsBValue, int dsCValue)
                 { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);

        dsE->set([](int dsBValue, int dsCValue, int dsDValue)
                 { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);

        dsF->set([](int aa, int bb)
                 { return aa + bb; }, a, b);

        dsG->set([](int dsAValue, int dsFValue)
                 { return dsAValue + dsFValue; }, dsA, dsF);
    }

    EXPECT_FALSE(static_cast<bool>(dsB));
    EXPECT_FALSE(static_cast<bool>(dsC));
    EXPECT_FALSE(static_cast<bool>(dsD));
    EXPECT_FALSE(static_cast<bool>(dsE));
    EXPECT_TRUE(static_cast<bool>(dsF));
    EXPECT_FALSE(static_cast<bool>(dsG));
}

TEST(TestKeepCalculateStrategy, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    a->setName("a");

    auto b = reaction::makeMetaDataSource(2);
    b->setName("b");

    auto dsB = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsC = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsD = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsE = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    dsB->setName("dsB");
    dsC->setName("dsC");
    dsD->setName("dsD");
    dsE->setName("dsE");

    {
        auto dsA = reaction::makeVariableDataSource<reaction::AlwaysTrigger, reaction::KeepCalculateStrategy>([](int aa)
                                                                                                              { return aa; }, a);
        dsA->setName("dsA");

        dsB->set([](int aa, int dsAValue)
                 { return aa + dsAValue; }, a, dsA);

        dsC->set([](int aa, int dsAValue, int dsBValue)
                 { return aa + dsAValue + dsBValue; }, a, dsA, dsB);

        dsD->set([](int dsAValue, int dsBValue, int dsCValue)
                 { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);

        dsE->set([](int dsBValue, int dsCValue, int dsDValue)
                 { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    }

    EXPECT_EQ(dsB->get(), 2);
    EXPECT_EQ(dsC->get(), 4);
    EXPECT_EQ(dsD->get(), 7);
    EXPECT_EQ(dsE->get(), 15);

    *a = 10;
    EXPECT_EQ(dsB->get(), 20);
    EXPECT_EQ(dsC->get(), 40);
    EXPECT_EQ(dsD->get(), 70);
    EXPECT_EQ(dsE->get(), 870);
}

TEST(TestUseLastValidValueStrategy, ReactionTest)
{
    auto a = reaction::makeMetaDataSource(1);
    a->setName("a");

    auto b = reaction::makeMetaDataSource(2);
    b->setName("b");

    auto dsB = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsC = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsD = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    auto dsE = reaction::makeVariableDataSource([](auto aa)
                                                { return 1; }, a);
    dsB->setName("dsB");
    dsC->setName("dsC");
    dsD->setName("dsD");
    dsE->setName("dsE");

    {
        auto dsA = reaction::makeVariableDataSource<reaction::AlwaysTrigger, reaction::UseLastValidValueStrategy>([](int aa)
                                                                                                                  { return aa; }, a);
        dsA->setName("dsA");

        dsB->set([](int aa, int dsAValue)
                 { return aa + dsAValue; }, a, dsA);

        dsC->set([](int aa, int dsAValue, int dsBValue)
                 { return aa + dsAValue + dsBValue; }, a, dsA, dsB);

        dsD->set([](int dsAValue, int dsBValue, int dsCValue)
                 { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);

        dsE->set([](int dsBValue, int dsCValue, int dsDValue)
                 { return dsBValue + dsCValue + dsDValue; }, dsB, dsC, dsD);
    }

    EXPECT_EQ(dsB->get(), 2);
    EXPECT_EQ(dsC->get(), 4);
    EXPECT_EQ(dsD->get(), 7);
    EXPECT_EQ(dsE->get(), 13);

    *a = 10;
    EXPECT_EQ(dsB->get(), 11);
    EXPECT_EQ(dsC->get(), 22);
    EXPECT_EQ(dsD->get(), 34);
    EXPECT_EQ(dsE->get(), 67);
}

struct Person
{
    int age;
    std::string name;

    Person(int a, std::string n) : age(a), name(n) {}

    Person(const Person &p)
    {
        age = p.age;
        name = p.name;
    }

    Person(Person &&p)
    {
        age = p.age;
        name = p.name;
    }

    Person &operator=(const Person &p)
    {
        age = p.age;
        name = p.name;
        return *this;
    }

    Person &operator=(const Person &&p)
    {
        age = p.age;
        name = p.name;
        return *this;
    }

    bool operator==(Person &p)
    {
        return name == p.name;
    }
};
TEST(TestCustomStruct, ReactionTest)
{
    Person p{18, "lummy"};
    auto a = reaction::makeMetaDataSource(p);
    auto ds = reaction::makeVariableDataSource([](auto &&aa)
                                               { return aa; }, a);
}

// 自定义数据类型用于最终转换
struct ProcessedData
{
    std::string info;
    int checksum;

    bool operator==(ProcessedData &p)
    {
        return info == p.info && checksum == p.checksum;
    }
};

TEST(DataSourceStressTest, DeepDependencyChain)
{
    using namespace reaction;
    using namespace std::chrono;

    // 1. 创建基础数据源
    auto base1 = makeMetaDataSource(1);    // int
    auto base2 = makeMetaDataSource(2.0);  // double
    auto base3 = makeMetaDataSource(true); // bool

    // 2. 构建10层依赖链
    auto layer1 = makeVariableDataSource([](int a, double b)
                                         {
                                             return a + b; // int + double → double
                                         },
                                         base1, base2);

    auto layer2 = makeVariableDataSource([](double val, bool flag)
                                         {
                                             return flag ? val * 2 : val / 2; // 条件计算 → double
                                         },
                                         layer1, base3);

    auto layer3 = makeVariableDataSource([](double val)
                                         {
                                             return "Value:" + std::to_string(val); // double → string
                                         },
                                         layer2);

    auto layer4 = makeVariableDataSource([](const std::string &s, int a)
                                         {
                                             return s + "_" + std::to_string(a); // string + int → string
                                         },
                                         layer3, base1);

    auto layer5 = makeVariableDataSource([](const std::string &s)
                                         {
                                             return s.length(); // string → size_t
                                         },
                                         layer4);

    auto layer6 = makeVariableDataSource([](size_t len, double b)
                                         {
                                             return std::vector<double>(len, b); // 生成向量
                                         },
                                         layer5, base2);

    auto layer7 = makeVariableDataSource([](const std::vector<double> &vec)
                                         {
                                             return std::accumulate(vec.begin(), vec.end(), 0.0); // 向量求和 → double
                                         },
                                         layer6);

    auto layer8 = makeVariableDataSource([](double sum, const std::string &info)
                                         {
                                             return ProcessedData{info, static_cast<int>(sum)}; // 自定义结构体
                                         },
                                         layer7, layer4);

    auto layer9 = makeVariableDataSource([](const ProcessedData &data)
                                         {
                                             return data.info + "|" + std::to_string(data.checksum); // 结构体 → string
                                         },
                                         layer8);

    auto finalLayer = makeVariableDataSource([](const std::string &s)
                                             {
                                                 return "Final:" + s; // 最终包装
                                             },
                                             layer9);

    // 3. 压力测试参数
    const int ITERATIONS = 100000;
    auto start = steady_clock::now();

    // 4. 高频更新测试
    for (int i = 0; i < ITERATIONS; ++i)
    {
        // 随机更新基础数据源
        *base1 = i % 100;
        *base2 = (i % 100) * 0.1;
        *base3 = i % 2 == 0;

        std::string expected = [&]() {
            // 完全重现数据源的计算步骤
            double l1 = base1->get() + base2->get();
            double l2 = base3->get() ? l1 * 2 : l1 / 2;
            std::string l3 = "Value:" + std::to_string(l2);
            std::string l4 = l3 + "_" + std::to_string(base1->get());
            size_t l5 = l4.length();
            std::vector<double> l6(l5, base2->get());
            double l7 = std::accumulate(l6.begin(), l6.end(), 0.0);
            ProcessedData l8{l4, static_cast<int>(l7)};
            std::string l9 = l8.info + "|" + std::to_string(l8.checksum);
            return "Final:" + l9;
        }();

        ASSERT_EQ(finalLayer->get(), expected);

        // 每1万次输出进度
        if (i % 10000 == 0)
        {
            auto dur = duration_cast<milliseconds>(steady_clock::now() - start);
            std::cout << "Progress: " << i << "/" << ITERATIONS
                      << " (" << dur.count() << "ms)\n";
        }
    }

    // 5. 性能统计
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);
    std::cout << "=== Stress Test Results ===\n"
              << "Iterations: " << ITERATIONS << "\n"
              << "Total time: " << duration.count() << "ms\n"
              << "Avg time per update: "
              << duration.count() / static_cast<double>(ITERATIONS) << "ms\n";
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}