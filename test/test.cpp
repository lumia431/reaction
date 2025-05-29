#include "reaction/react.h"
#include "gtest/gtest.h"
#include <chrono>
#include <numeric>

TEST(ReactionTest, TestCalac) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc([](int aa, double dsds) { return std::to_string(aa) + std::to_string(dsds); }, a, ds);

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");
}

TEST(ReactionTest, TestTrigger) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + std::to_string(dsds); }, a, ds);

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");

    a.value(2);
    ASSERT_FLOAT_EQ(ds.get(), 5.14);
    EXPECT_EQ(dds.get(), "25.140000");
}

TEST(ReactionTest, TestCopy) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);

    auto dds_copy = dds;
    EXPECT_EQ(dds_copy.get(), "113.140000");
    EXPECT_EQ(dds.get(), "113.140000");

    a.value(2);
    EXPECT_EQ(dds_copy.get(), "223.140000");
    EXPECT_EQ(dds.get(), "223.140000");
}

TEST(ReactionTest, TestMove) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);

    auto dds_move = std::move(dds);
    EXPECT_EQ(dds_move.get(), "113.140000");
    EXPECT_FALSE(static_cast<bool>(dds));
    EXPECT_THROW(dds.get(), std::runtime_error);

    a.value(2);
    EXPECT_EQ(dds_move.get(), "223.140000");
    EXPECT_FALSE(static_cast<bool>(dds));
}

TEST(ReactionTest, TestConst) {
    auto a = reaction::var(1);
    auto b = reaction::constVar(3.14);
    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    ASSERT_FLOAT_EQ(ds.get(), 4.14);

    a.value(2);
    ASSERT_FLOAT_EQ(ds.get(), 5.14);
    // b.value(4.14); // compile error;
}

TEST(ReactionTest, TestAction) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto at = reaction::action([](int aa, double bb) { std::cout << "a = " << aa << '\t' << "b = " << bb << '\t'; }, a, b);

    bool trigger = false;
    auto att = reaction::action([&]([[maybe_unused]] auto atat) {
        trigger = true;
        std::cout << "at trigger " << std::endl;
    },
        at);

    trigger = false;

    a.value(2);
    EXPECT_TRUE(trigger);
}

class Person : public reaction::FieldBase {
public:
    Person(std::string name, int age, bool male) : m_name(field(name)), m_age(field(age)), m_male(male) {
    }

    std::string getName() const {
        return m_name.get();
    }
    void setName(const std::string &name) {
        *m_name = name;
    }

    int getAge() const {
        return m_age.get();
    }
    void setAge(int age) {
        *m_age = age;
    }

private:
    reaction::Field<std::string> m_name;
    reaction::Field<int> m_age;
    bool m_male;
};

TEST(ReactionTest, TestField) {
    Person person{"lummy", 18, true};
    auto p = reaction::var(person);
    auto a = reaction::var(1);
    auto ds = reaction::calc([](int aa, auto pp) { return std::to_string(aa) + pp.getName(); }, a, p);

    EXPECT_EQ(ds.get(), "1lummy");
    p->setName("lummy-new");
    EXPECT_EQ(ds.get(), "1lummy-new");
}

TEST(ReactionTest, TestReset) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);

    auto ds = reaction::calc([](auto aa, auto bb) { return aa + bb; }, a, b);

    auto dds = reaction::calc([](auto aa, auto bb) { return aa + bb; }, a, b);

    dds.reset([](auto aa, auto dsds) { return aa + dsds; }, a, ds);
    a.value(2);
    EXPECT_EQ(dds.get(), 6);
}

TEST(ReactionTest, TestParentheses) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([&]() { return a() + b(); });
    auto dds = reaction::calc([&]() { return std::to_string(a()) + std::to_string(ds()); });

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");

    a.value(2);
    ASSERT_FLOAT_EQ(ds.get(), 5.14);
    EXPECT_EQ(dds.get(), "25.140000");
}

TEST(ReactionTest, TestExpr) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3.14);
    auto ds = reaction::calc([&]() { return a() + b(); });
    auto expr_ds = reaction::expr(c + a / b - ds * 2);

    a.value(2);
    EXPECT_EQ(ds.get(), 4);
    ASSERT_FLOAT_EQ(expr_ds.get(), -3.86);
}

TEST(ReactionTest, TestSelfDependency) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    auto dsA = reaction::calc([](int aa) { return aa; }, a);

    EXPECT_EQ(dsA.reset([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA),
        reaction::ReactionError::CycleDepErr);
}

TEST(ReactionTest, TestCycleDependency) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    auto dsA = reaction::calc([](int bb) { return bb; }, b);

    auto dsB = reaction::calc([](int cc) { return cc; }, c);

    auto dsC = reaction::calc([](int aa) { return aa; }, a);

    dsA.reset([](int bb, int dsBValue) { return bb + dsBValue; }, b, dsB);

    dsB.reset([](int cc, int dsCValue) { return cc * dsCValue; }, c, dsC);

    EXPECT_EQ(dsC.reset([](int aa, int dsAValue) { return aa - dsAValue; }, a, dsA),
        reaction::ReactionError::CycleDepErr);
}

TEST(ReactionTest, TestRepeatDependency) {
    // ds → A, ds → a, A → a
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(2).setName("b");

    int triggerCount = 0;
    auto dsA = reaction::calc([&]() { return a() + b(); }).setName("dsA");

    auto dsB = reaction::calc([&]() {++triggerCount; return a() + dsA(); }).setName("dsB");

    triggerCount = 0;
    *a = 2;
    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(dsB.get(), 6);
}

TEST(ReactionTest, TestRepeatDependency2) {
    // ds → A, ds → B, ds → C, A → a, B → a
    int triggerCount = 0;
    auto a = reaction::var(1).setName("a");
    auto A = reaction::calc([&]() { return a() + 1; }).setName("A");
    auto B = reaction::calc([&]() { return a() + 2; }).setName("B");
    auto C = reaction::calc([&]() { return 5; }).setName("C");
    auto ds = reaction::calc([&]() { ++triggerCount; return A() + B() + C(); }).setName("ds");

    triggerCount = 0;
    *a = 2;
    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(ds.get(), 12);
}

TEST(ReactionTest, TestRepeatDependency3) {
    // ds → A, ds → B, A → A1, A1 → A2, A2 → a, B → B1, B1 → a
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(1).setName("b");

    int triggerCount = 0;
    auto A2 = reaction::calc([&]() { return a() * 2; }).setName("A2");
    auto A1 = reaction::calc([&]() { return A2() + 1; }).setName("A1");
    auto A = reaction::calc([&]() { return A1() - 1; }).setName("A");

    auto B1 = reaction::calc([&]() { return a() - 1; }).setName("B1");
    auto B = reaction::calc([&]() { return B1() + 1; }).setName("B");

    auto ds = reaction::calc([&]() { ++triggerCount; return A() + B(); }).setName("ds");
    triggerCount = 0;
    *a = 2;
    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(ds.get(), 6);

    A1.reset([](auto bb) { return bb; }, b);
    EXPECT_EQ(ds.get(), 2);
}

TEST(ReactionTest, TestChangeTrig) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto c = reaction::var("cc");
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds = reaction::calc([&triggerCountA](int aa, double bb) {
                                                ++triggerCountA;
                                                return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc<reaction::ChangeTrig>([&triggerCountB](auto cc, auto dsds) {
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

TEST(ReactionTest, TestFilterTrig) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);
    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc<reaction::FilterTrig>([](auto cc, auto dsds) { return cc + dsds; }, c, ds);
    *a = 2;
    EXPECT_EQ(ds.get(), 4);
    EXPECT_EQ(dds.get(), 7);

    dds.filter([&]() { return c() + ds() < 10; });
    *a = 3;
    EXPECT_EQ(dds.get(), 8);

    *a = 5;
    EXPECT_EQ(dds.get(), 8);
}

TEST(ReactionTest, TestCloseStra) {
    auto a = reaction::var(1);
    a.setName("a");

    auto b = reaction::var(2);
    b.setName("b");

    auto dsB = reaction::calc([](auto aa) { return aa; }, a);
    auto dsC = reaction::calc([](auto aa) { return aa; }, a);
    auto dsD = reaction::calc([](auto aa) { return aa; }, a);
    auto dsE = reaction::calc([](auto aa) { return aa; }, a);
    auto dsF = reaction::calc([](auto aa) { return aa; }, a);
    auto dsG = reaction::calc([](auto aa) { return aa; }, a);
    dsB.setName("dsB");
    dsC.setName("dsC");
    dsD.setName("dsD");
    dsE.setName("dsE");
    dsF.setName("dsF");
    dsG.setName("dsG");

    {
        auto dsA = reaction::calc([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.reset([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);
        dsC.reset([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
        dsD.reset([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
        dsE.reset([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
        dsF.reset([](int aa, int bb) { return aa + bb; }, a, b);
        dsG.reset([](int dsAValue, int dsFValue) { return dsAValue + dsFValue; }, dsA, dsF);
    }

    EXPECT_FALSE(static_cast<bool>(dsB));
    EXPECT_FALSE(static_cast<bool>(dsC));
    EXPECT_FALSE(static_cast<bool>(dsD));
    EXPECT_FALSE(static_cast<bool>(dsE));
    EXPECT_TRUE(static_cast<bool>(dsF));
    EXPECT_FALSE(static_cast<bool>(dsG));
}

TEST(ReactionTest, TestKeepStra) {
    auto a = reaction::var(1);
    a.setName("a");

    auto b = reaction::var(2);
    b.setName("b");

    auto dsB = reaction::calc([](auto aa) { return aa; }, a);
    auto dsC = reaction::calc([](auto aa) { return aa; }, a);
    auto dsD = reaction::calc([](auto aa) { return aa; }, a);
    auto dsE = reaction::calc([](auto aa) { return aa; }, a);
    dsB.setName("dsB");
    dsC.setName("dsC");
    dsD.setName("dsD");
    dsE.setName("dsE");

    {
        auto dsA = reaction::calc<reaction::ChangeTrig, reaction::KeepStra>([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.reset([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);
        dsC.reset([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
        dsD.reset([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
        dsE.reset([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    }

    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);
    EXPECT_EQ(dsD.get(), 7);
    EXPECT_EQ(dsE.get(), 15);

    *a = 10;
    EXPECT_EQ(dsB.get(), 20);
    EXPECT_EQ(dsC.get(), 40);
    EXPECT_EQ(dsD.get(), 70);
    EXPECT_EQ(dsE.get(), 870);
}

TEST(ReactionTest, TestLastStra) {
    auto a = reaction::var(1);
    a.setName("a");

    auto b = reaction::var(2);
    b.setName("b");

    auto dsB = reaction::calc([](auto aa) { return aa; }, a);
    auto dsC = reaction::calc([](auto aa) { return aa; }, a);
    auto dsD = reaction::calc([](auto aa) { return aa; }, a);
    auto dsE = reaction::calc([](auto aa) { return aa; }, a);
    dsB.setName("dsB");
    dsC.setName("dsC");
    dsD.setName("dsD");
    dsE.setName("dsE");

    {
        auto dsA = reaction::calc<reaction::ChangeTrig, reaction::LastStra>([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.reset([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);
        dsC.reset([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
        dsD.reset([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
        dsE.reset([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue + dsCValue + dsDValue; }, dsB, dsC, dsD);
    }

    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);
    EXPECT_EQ(dsD.get(), 7);
    EXPECT_EQ(dsE.get(), 13);

    *a = 10;
    EXPECT_EQ(dsB.get(), 11);
    EXPECT_EQ(dsC.get(), 22);
    EXPECT_EQ(dsD.get(), 34);
    EXPECT_EQ(dsE.get(), 67);
}

// struct ProcessedData {
//     std::string info;
//     int checksum;
// };

// TEST(ReactionTest, StressTest) {
//     using namespace reaction;
//     using namespace std::chrono;

//     // Create var-data sources
//     auto base1 = var(1);                // Integer source
//     auto base2 = var(2.0);              // Double source
//     auto base3 = var(true);             // Boolean source
//     auto base4 = var(std::string{"3"}); // String source
//     auto base5 = var(4);                // Integer source

//     // Layer 1: Add integer and double
//     auto layer1 = calc([](int a, double b) {
//         return a + b;
//     },
//         base1, base2);

//     // Layer 2: Multiply or divide based on the flag
//     auto layer2 = calc([](double val, bool flag) {
//         return flag ? val * 2 : val / 2;
//     },
//         layer1, base3);

//     // Layer 3: Convert double value to a string
//     auto layer3 = calc([](double val) {
//         return "Value:" + std::to_string(val);
//     },
//         layer2);

//     // Layer 4: Append integer to string
//     auto layer4 = calc([](const std::string &s, const std::string &s4) {
//         return s + "_" + s4;
//     },
//         layer3, base4);

//     // Layer 5: Get the length of the string
//     auto layer5 = calc([](const std::string &s) {
//         return s.length();
//     },
//         layer4);

//     // Layer 6: Create a vector of double values
//     auto layer6 = calc([](size_t len, int b5) {
//         return std::vector<int>(len, b5);
//     },
//         layer5, base5);

//     // Layer 7: Sum all elements in the vector
//     auto layer7 = calc([](const std::vector<int> &vec) {
//         return std::accumulate(vec.begin(), vec.end(), 0);
//     },
//         layer6);

//     // Layer 8: Create a ProcessedData object with checksum and info
//     auto layer8 = calc([](int sum) {
//         return ProcessedData{"ProcessedData", static_cast<int>(sum)};
//     },
//         layer7);

//     // Layer 9: Combine info and checksum into a string
//     auto layer9 = calc([](const ProcessedData &calc) {
//         return calc.info + "|" + std::to_string(calc.checksum);
//     },
//         layer8);

//     // Final layer: Add "Final:" prefix to the result
//     auto finalLayer = calc([](const std::string &s) {
//         return "Final:" + s;
//     },
//         layer9);

//     const int ITERATIONS = 1000000;
//     auto start = steady_clock::now(); // Start measuring time
//     // Perform stress test for the given number of iterations
//     for (int i = 0; i < ITERATIONS; ++i) {
//         // Update base sources with new values
//         base1.value(i % 100);
//         base2.value((i % 100) * 0.1);
//         base3.value(i % 2 == 0);

//         // Calculate the expected result for the given input
//         std::string expected = [&]() {
//             double l1 = base1.get() + base2.get();                        // Add base1 and base2
//             double l2 = base3.get() ? l1 * 2 : l1 / 2;                    // Multiply or divide based on base3
//             std::string l3 = "Value:" + std::to_string(l2);               // Convert to string
//             std::string l4 = l3 + "_" + base4.get();                      // Append base1
//             size_t l5 = l4.length();                                      // Get string length
//             std::vector<int> l6(l5, base5.get());                         // Create vector of length 'l5'
//             int l7 = std::accumulate(l6.begin(), l6.end(), 0);            // Sum vector values
//             ProcessedData l8{"ProcessedData", static_cast<int>(l7)};      // Create ProcessedData object
//             std::string l9 = l8.info + "|" + std::to_string(l8.checksum); // Combine info and checksum
//             return "Final:" + l9;                                         // Add final prefix
//         }();

//         // Print progress every 10,000 iterations
//         if (i % 10000 == 0 && finalLayer.get() == expected) {
//             auto dur = duration_cast<milliseconds>(steady_clock::now() - start);
//             std::cout << "Progress: " << i << "/" << ITERATIONS
//                       << " (" << dur.count() << "ms)\n";
//         }
//     }

//     // Output the final results of the stress test
//     auto duration = duration_cast<milliseconds>(steady_clock::now() - start);
//     std::cout << "=== Stress Test Results ===\n"
//               << "Iterations: " << ITERATIONS << "\n"
//               << "Total time: " << duration.count() << "ms\n"
//               << "Avg time per update: "
//               << duration.count() / static_cast<double>(ITERATIONS) << "ms\n";
// }

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
