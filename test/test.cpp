/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "gtest/gtest.h"
#include "reaction/dataSource.h"
#include <vector>
#include <numeric>

// Test for basic constructor and data source creation
TEST(TestConstructor, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);

    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);

    EXPECT_EQ(ds.get(), "13.140000");
    EXPECT_EQ(dds.get(), "113.140000");
}

// Test common usage of variable data source
TEST(TestCommonUse, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto c = reaction::var(5);
    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);
    auto simple_ds = reaction::calc([=]() { return a() + b(); });
    auto expr_ds = reaction::expr(a + b);
    auto expr_ds2 = reaction::expr(a + 1);
    auto expr_ds3 = reaction::expr(c + a * b - 3);

    a.value(2);
    EXPECT_EQ(ds.get(), "23.140000");
    EXPECT_EQ(dds.get(), "223.140000");
    ASSERT_FLOAT_EQ(simple_ds.get(), 5.14);
    ASSERT_FLOAT_EQ(expr_ds.get(), 5.14);
    EXPECT_EQ(expr_ds2.get(), 3);
    ASSERT_FLOAT_EQ(expr_ds3.get(), 8.28);
}

// Test for complex calculations with multiple dependencies
TEST(TestComplexCal, ReactionTest) {
    auto a = reaction::var(1);
    a.setName("a");

    auto dsA = reaction::calc([](int aa) { return aa; }, a);
    dsA.setName("dsA");

    auto dsB = reaction::calc([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);
    dsB.setName("dsB");

    auto dsC = reaction::calc([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
    dsC.setName("dsC");

    auto dsD = reaction::calc([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
    dsD.setName("dsD");

    auto dsE = reaction::calc([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    dsE.setName("dsE");

    EXPECT_EQ(dsA.get(), 1);
    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);
    EXPECT_EQ(dsD.get(), 7);
    EXPECT_EQ(dsE.get(), 15);

    *a = 10;
    EXPECT_EQ(dsA.get(), 10);
    EXPECT_EQ(dsB.get(), 20);
    EXPECT_EQ(dsC.get(), 40);
    EXPECT_EQ(dsD.get(), 70);
    EXPECT_EQ(dsE.get(), 870);
}

// Test for constant data sources (values cannot be updated)
TEST(TestConstDataSource, ReactionTest) {
    auto a = reaction::constVar(1);
    auto b = reaction::constVar(3.14);
    // *a = 2; // This should cause a compilation error as a is constant
    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);
}

// Test for action nodes (testing side effects of actions)
TEST(TestAction, ActionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto ds = reaction::calc([](auto aa, auto bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);

    int val = 10;
    auto dds = reaction::action([&val](auto aa) { val = aa; }, a);

    EXPECT_EQ(val, 1); // Initial value set by the action
    *a = 2;            // Trigger the action
    EXPECT_EQ(val, 2); // Value should now be updated to 2
}

// Test for resetting nodes and dependencies
TEST(TestReset, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(std::string{"2"});
    auto c = reaction::var(std::string{"3"});
    auto d = reaction::var(std::string{"4"});

    auto ds = reaction::calc([](auto aa) { return std::to_string(aa); }, a);

    auto dds = reaction::calc([](auto bb) { return bb; }, b);

    auto ddds = reaction::calc([](auto cc) { return cc; }, c);

    EXPECT_EQ(ddds.get(), "3");
    ddds.set([=]() { return d() + dds() + "set"; });
    EXPECT_EQ(ddds.get(), "42set");
    *c = "33";
    EXPECT_EQ(ddds.get(), "42set");
    *d = "44";
    EXPECT_EQ(ddds.get(), "442set");
}

// Test for self-dependency scenario (invalid)
TEST(TestSelfDependency, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    auto dsA = reaction::calc([](int aa) { return aa; }, a);

    EXPECT_EQ(dsA.set([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA),
              false); // This should fail as self-dependency is not allowed
}

// Test for repeat dependencies and the number of trigger counts
TEST(TestRepeatDependency, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    int triggerCount = 0;
    auto dsA = reaction::calc([&triggerCount](int aa, int bb) {
                                                    ++triggerCount;
                                                    return aa + bb; }, a, b);

    auto dsB = reaction::calc([](int cc, int dsAVal) { return cc + dsAVal; }, c, dsA);

    a.setName("a");
    b.setName("b");
    c.setName("c");

    dsA.setName("dsA");
    dsB.setName("dsB");

    triggerCount = 0;
    *a = 2;
    EXPECT_EQ(triggerCount, 1);

    dsB.set([](int aa, int dsAVal) { return aa + dsAVal; }, a, dsA);

    triggerCount = 0;
    *a = 3;
    EXPECT_EQ(triggerCount, 3); // Multiple triggers due to repeat dependency
}

// Test for cyclic dependencies, expecting failure
TEST(TestCycleDependency, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    auto dsA = reaction::calc([](int bb) { return bb; }, b);

    auto dsB = reaction::calc([](int cc) { return cc; }, c);

    auto dsC = reaction::calc([](int aa) { return aa; }, a);

    a.setName("a");
    b.setName("b");
    c.setName("c");

    dsA.setName("dsA");
    dsB.setName("dsB");
    dsC.setName("dsC");

    dsA.set([](int bb, int dsBValue) { return bb + dsBValue; }, b, dsB);

    dsB.set([](int cc, int dsCValue) { return cc * dsCValue; }, c, dsC);

    EXPECT_EQ(dsC.set([](int aa, int dsAValue) { return aa - dsAValue; }, a, dsA),
              false); // This should fail due to cycle dependency
}

// Test for copying data sources
TEST(TestCopy, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);

    auto dds_copy = dds;
    EXPECT_EQ(dds_copy.get(), "113.140000");
    EXPECT_EQ(dds.get(), "113.140000");

    *a = 2;
    EXPECT_EQ(dds_copy.get(), "223.140000");
    EXPECT_EQ(dds.get(), "223.140000");
}

// Test for moving data sources
TEST(TestMove, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto ds = reaction::calc([](int aa, double bb) { return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc([](auto aa, auto dsds) { return std::to_string(aa) + dsds; }, a, ds);

    auto dds_copy = std::move(dds);
    EXPECT_EQ(dds_copy.get(), "113.140000");
    EXPECT_FALSE(static_cast<bool>(dds));
    EXPECT_THROW(dds.get(), std::runtime_error);

    *a = 2;
    EXPECT_EQ(dds_copy.get(), "223.140000");
    EXPECT_FALSE(static_cast<bool>(dds));
}

// Test for value change trigger
TEST(TestValueChangeTrigger, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    auto c = reaction::var("cc");
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds = reaction::calc([&triggerCountA](int aa, double bb) {
                                                ++triggerCountA;
                                                return std::to_string(aa) + std::to_string(bb); }, a, b);
    auto dds = reaction::calc<reaction::ValueChangeTrigger>([&triggerCountB](auto cc, auto dsds) {
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

// Test for threshold trigger
TEST(TestThresholdTrigger, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds = reaction::calc([&triggerCountA](int aa, double bb) {
                                                ++triggerCountA;
                                                return aa + bb; }, a, b);
    auto dds = reaction::calc<reaction::ThresholdTrigger>([&triggerCountB](auto cc, auto dsds) {
                                                                             ++triggerCountB;
                                                                             return cc + dsds; }, c, ds);
    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(triggerCountB, 1);
    *a = 2;
    EXPECT_EQ(triggerCountA, 2);
    EXPECT_EQ(triggerCountB, 2);
    EXPECT_EQ(ds.get(), 4);
    EXPECT_EQ(dds.get(), 7);

    dds.setThreshold([=]() { return c() + ds() < 10; });
    *a = 5;
    EXPECT_EQ(triggerCountA, 3);
    EXPECT_EQ(triggerCountB, 2);
    EXPECT_EQ(dds.get(), 7);
}

// Test for closing nodes in a field source
TEST(TestClose, ReactionTest) {
    auto a = reaction::var(1);
    a.setName("a");

    auto b = reaction::var(2);
    b.setName("b");

    auto dsA = reaction::calc([](int aa) { return aa; }, a);
    dsA.setName("dsA");

    auto dsB = reaction::calc([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);
    dsB.setName("dsB");

    auto dsC = reaction::calc([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);
    dsC.setName("dsC");

    auto dsD = reaction::calc([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);
    dsD.setName("dsD");

    auto dsE = reaction::calc([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    dsE.setName("dsE");

    auto dsF = reaction::calc([](int aa, int bb) { return aa + bb; }, a, b);
    dsF.setName("dsF");

    auto dsG = reaction::calc([](int dsAValue, int dsFValue) { return dsAValue + dsFValue; }, dsA, dsF);
    dsG.setName("dsG");

    dsA.close();
    EXPECT_FALSE(static_cast<bool>(dsA));
    EXPECT_FALSE(static_cast<bool>(dsB));
    EXPECT_FALSE(static_cast<bool>(dsC));
    EXPECT_FALSE(static_cast<bool>(dsD));
    EXPECT_FALSE(static_cast<bool>(dsE));
    EXPECT_TRUE(static_cast<bool>(dsF));
    EXPECT_FALSE(static_cast<bool>(dsG));
}

// Test for DirectCloseStrategy, checking if nodes are closed when invalid
TEST(TestDirectFailureStrategy, ReactionTest) {
    auto a = reaction::var(1);
    a.setName("a");

    auto b = reaction::var(2);
    b.setName("b");

    // Create multiple calc sources
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

        dsB.set([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);

        dsC.set([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);

        dsD.set([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);

        dsE.set([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);

        dsF.set([](int aa, int bb) { return aa + bb; }, a, b);

        dsG.set([](int dsAValue, int dsFValue) { return dsAValue + dsFValue; }, dsA, dsF);
    }

    // Check that invalid sources (due to DirectCloseStrategy) have been closed
    EXPECT_FALSE(static_cast<bool>(dsB));
    EXPECT_FALSE(static_cast<bool>(dsC));
    EXPECT_FALSE(static_cast<bool>(dsD));
    EXPECT_FALSE(static_cast<bool>(dsE));
    EXPECT_TRUE(static_cast<bool>(dsF));
    EXPECT_FALSE(static_cast<bool>(dsG));
}

// Test for KeepCalcStrategy, ensuring values are recalculated even if invalid
TEST(TestKeepCalculateStrategy, ReactionTest) {
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
        auto dsA = reaction::calc<reaction::AlwaysTrigger, reaction::KeepCalcStrategy>([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.set([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);

        dsC.set([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);

        dsD.set([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);

        dsE.set([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue * dsCValue + dsDValue; }, dsB, dsC, dsD);
    }

    // Check the computed values before and after changing the input
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

// Test for LastValStrategy, ensuring last valid value is retained when invalid
TEST(TestUseLastValidValueStrategy, ReactionTest) {
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
        auto dsA = reaction::calc<reaction::AlwaysTrigger, reaction::LastValStrategy>([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.set([](int aa, int dsAValue) { return aa + dsAValue; }, a, dsA);

        dsC.set([](int aa, int dsAValue, int dsBValue) { return aa + dsAValue + dsBValue; }, a, dsA, dsB);

        dsD.set([](int dsAValue, int dsBValue, int dsCValue) { return dsAValue + dsBValue + dsCValue; }, dsA, dsB, dsC);

        dsE.set([](int dsBValue, int dsCValue, int dsDValue) { return dsBValue + dsCValue + dsDValue; }, dsB, dsC, dsD);
    }

    // Check the computed values before and after changing the input
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

// Test custom struct with reaction data sources
struct Person {
    int age;
    std::string name;

    Person(int a, std::string n) :
        age(a), name(n) {
    }

    Person(const Person &p) {
        age = p.age;
        name = p.name;
    }

    Person(Person &&p) {
        age = p.age;
        name = p.name;
    }

    Person &operator=(const Person &p) {
        age = p.age;
        name = p.name;
        return *this;
    }

    Person &operator=(const Person &&p) {
        age = p.age;
        name = p.name;
        return *this;
    }

    bool operator==(Person &p) {
        return name == p.name;
    }
};

// Test for custom struct with reaction
TEST(TestCustomStruct, ReactionTest) {
    Person p{18, "lummy"};
    auto a = reaction::var(p);
    auto ds = reaction::calc([](auto &&aa) { return aa; }, a);
}

struct ProcessedData {
    std::string info; // Stores information
    int checksum;     // Stores the checksum value

    // Overloading equality operator for comparison
    bool operator==(ProcessedData &p) {
        return info == p.info && checksum == p.checksum;
    }
};

// Test case for a deep dependency chain in data sources
TEST(DataSourceStressTest, DeepDependencyChain) {
    using namespace reaction;
    using namespace std::chrono;

    // Create var-data sources
    auto base1 = var(1);                // Integer source
    auto base2 = var(2.0);              // Double source
    auto base3 = var(true);             // Boolean source
    auto base4 = var(std::string{"3"}); // String source
    auto base5 = var(4);                // Integer source

    // Layer 1: Add integer and double
    auto layer1 = calc([](int a, double b) {
        return a + b;
    },
                                         base1, base2);

    // Layer 2: Multiply or divide based on the flag
    auto layer2 = calc([](double val, bool flag) {
        return flag ? val * 2 : val / 2;
    },
                                         layer1, base3);

    // Layer 3: Convert double value to a string
    auto layer3 = calc([](double val) {
        return "Value:" + std::to_string(val);
    },
                                         layer2);

    // Layer 4: Append integer to string
    auto layer4 = calc([](const std::string &s, const std::string &s4) {
        return s + "_" + s4;
    },
                                         layer3, base4);

    // Layer 5: Get the length of the string
    auto layer5 = calc([](const std::string &s) {
        return s.length();
    },
                                         layer4);

    // Layer 6: Create a vector of double values
    auto layer6 = calc([](size_t len, int b5) {
        return std::vector<int>(len, b5);
    },
                                         layer5, base5);

    // Layer 7: Sum all elements in the vector
    auto layer7 = calc([](const std::vector<int> &vec) {
        return std::accumulate(vec.begin(), vec.end(), 0);
    },
                                         layer6);

    // Layer 8: Create a ProcessedData object with checksum and info
    auto layer8 = calc([](int sum) {
        return ProcessedData{"ProcessedData", static_cast<int>(sum)};
    },
                                         layer7);

    // Layer 9: Combine info and checksum into a string
    auto layer9 = calc([](const ProcessedData &calc) {
        return calc.info + "|" + std::to_string(calc.checksum);
    },
                                         layer8);

    // Final layer: Add "Final:" prefix to the result
    auto finalLayer = calc([](const std::string &s) {
        return "Final:" + s;
    },
                                             layer9);
    const int ITERATIONS = 1;
    auto start = steady_clock::now(); // Start measuring time
    // Perform stress test for the given number of iterations
    for (int i = 0; i < ITERATIONS; ++i) {
        // Update base sources with new values
        *base1 = i % 100;
        *base2 = (i % 100) * 0.1;
        *base3 = i % 2 == 0;

        // Calculate the expected result for the given input
        std::string expected = [&]() {
            double l1 = base1.get() + base2.get();                        // Add base1 and base2
            double l2 = base3.get() ? l1 * 2 : l1 / 2;                    // Multiply or divide based on base3
            std::string l3 = "Value:" + std::to_string(l2);               // Convert to string
            std::string l4 = l3 + "_" + base4.get();                      // Append base1
            size_t l5 = l4.length();                                      // Get string length
            std::vector<int> l6(l5, base5.get());                         // Create vector of length 'l5'
            int l7 = std::accumulate(l6.begin(), l6.end(), 0);            // Sum vector values
            ProcessedData l8{"ProcessedData", static_cast<int>(l7)};      // Create ProcessedData object
            std::string l9 = l8.info + "|" + std::to_string(l8.checksum); // Combine info and checksum
            return "Final:" + l9;                                         // Add final prefix
        }();

        // Check if the final result matches the expected value
        EXPECT_EQ(finalLayer.get(), expected);

        // Print progress every 10,000 iterations
        if (i % 10000 == 0) {
            auto dur = duration_cast<milliseconds>(steady_clock::now() - start);
            std::cout << "Progress: " << i << "/" << ITERATIONS
                      << " (" << dur.count() << "ms)\n";
        }
    }

    // Output the final results of the stress test
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);
    std::cout << "=== Stress Test Results ===\n"
              << "Iterations: " << ITERATIONS << "\n"
              << "Total time: " << duration.count() << "ms\n"
              << "Avg time per update: "
              << duration.count() / static_cast<double>(ITERATIONS) << "ms\n";
}

// Person class with fields for name, age, and gender
class PersonField : public reaction::FieldStructBase {
public:
    // Constructor to initialize PersonField with name, age, and gender
    PersonField(std::string name, int age, bool male) :
        m_name(reaction::field(this, name)), m_age(reaction::field(this, age)), m_male(male) {
    }

    // Copy constructor
    PersonField(const PersonField &p) :
        m_name(reaction::field(this, p.m_name.get())), m_age(reaction::field(this, p.m_age.get())), m_male(p.m_male) {
    }

    // Move constructor
    PersonField(PersonField &&p) :
        m_name(reaction::field(this, p.m_name.get())), m_age(reaction::field(this, p.m_age.get())), m_male(p.m_male) {
    }

    // Overloading equality operator for comparison
    bool operator==(PersonField &p) {
        return m_name.get() == p.m_name.get();
    }

    // Getter and setter for name
    std::string getName() {
        return m_name.get();
    }
    void setName(const std::string &name) {
        *m_name = name;
    }

    // Getter and setter for age
    int getAge() {
        return m_age.get();
    }
    void setAge(int age) {
        *m_age = age;
    }

private:
    // Field sources for name and age
    reaction::Field<std::string> m_name;
    reaction::Field<int> m_age;
    bool m_male; // Gender field
};

// Test for PersonField class
TEST(TestFieldSource, ReactionTest) {
    // Create a PersonField object
    PersonField person{"lummy", 18, true};
    auto p = reaction::var(person);
    auto a = reaction::var(1);

    // Create a data source that combines the integer 'a' and the person's name
    auto ds = reaction::calc([](int aa, auto pp) { return std::to_string(aa) + pp.getName(); }, a, p);

    // Verify the result
    EXPECT_EQ(ds.get(), "1lummy");

    // Update person's name and verify the updated result
    p->setName("lummy-new");
    EXPECT_EQ(ds.get(), "1lummy-new");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
