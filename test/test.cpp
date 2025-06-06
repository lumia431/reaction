/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "reaction/reaction.h"
#include "gtest/gtest.h"
#include <chrono>
#include <list>
#include <numeric>
#include <set>

TEST(ReactionTest, TestCalac) {
    auto a = reaction::var(1);
    auto b = reaction::var(3.14);
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 3.14);

    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc([](int aa, double dsds) { return std::to_string(aa) + std::to_string(dsds); }, a, ds);
    auto ddds = reaction::calc([]() { return 1; });

    ASSERT_FLOAT_EQ(ds.get(), 4.14);
    EXPECT_EQ(dds.get(), "14.140000");
    EXPECT_EQ(ddds.get(), 1);
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
        m_name.value(name);
    }

    int getAge() const {
        return m_age.get();
    }
    void setAge(int age) {
        m_age.value(age);
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

    dds.reset([&]() { return a() + ds(); });
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
    auto dsA = reaction::calc([](int aa) { return aa; }, a);

    EXPECT_THROW(dsA.reset([&]() { return a() + dsA(); }), std::runtime_error);
}

TEST(ReactionTest, TestCycleDependency) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);

    auto dsA = reaction::calc([](int bb) { return bb; }, b);

    auto dsB = reaction::calc([](int cc) { return cc; }, c);

    auto dsC = reaction::calc([](int aa) { return aa; }, a);

    dsA.reset([&]() { return b() + dsB(); });

    dsB.reset([&]() { return c() * dsC(); });

    EXPECT_THROW(dsC.reset([&]() { return a() - dsA(); }), std::runtime_error);
}

TEST(ReactionTest, TestRepeatDependency) {
    // ds → A, ds → a, A → a
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(2).setName("b");

    int triggerCount = 0;
    auto dsA = reaction::calc([&]() { return a() + b(); }).setName("dsA");

    auto dsB = reaction::calc([&]() {++triggerCount; return a() + dsA(); }).setName("dsB");

    triggerCount = 0;
    a.value(2);
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
    a.value(2);
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
    a.value(2);
    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(ds.get(), 6);

    A1.reset([&]() { return b(); });
    EXPECT_EQ(ds.get(), 2);
}

TEST(ReactionTest, TestChangeTrig) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds = reaction::calc<reaction::AlwaysTrig>([&triggerCountA](int aa, double bb) { ++triggerCountA; return aa + bb; }, a, b);
    auto dds = reaction::calc([&triggerCountB](auto aa, auto cc) { ++triggerCountB; return aa + cc; }, a, c);
    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(triggerCountB, 1);
    a.value(1);
    EXPECT_EQ(triggerCountA, 2);
    EXPECT_EQ(triggerCountB, 1);

    a.value(2);
    EXPECT_EQ(triggerCountA, 3);
    EXPECT_EQ(triggerCountB, 2);
}

TEST(ReactionTest, TestFilterTrig) {
    auto a = reaction::var(1);
    auto b = reaction::var(2);
    auto c = reaction::var(3);
    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds = reaction::calc<reaction::FilterTrig>([](auto cc, auto dsds) { return cc + dsds; }, c, ds);
    a.value(2);
    EXPECT_EQ(ds.get(), 4);
    EXPECT_EQ(dds.get(), 7);

    dds.filter([&]() { return c() + ds() < 10; });
    a.value(3);
    EXPECT_EQ(dds.get(), 8);

    a.value(5);
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
        auto dsA = reaction::calc<reaction::ChangeTrig, reaction::CloseStra>([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.reset([&]() { return a() + dsA(); });
        dsC.reset([&]() { return a() + dsA() + dsB(); });
        dsD.reset([&]() { return dsA() + dsB() + dsC(); });
        dsE.reset([&]() { return dsB() * dsC() + dsD(); });
        dsF.reset([&]() { return a() + b(); });
        dsG.reset([&]() { return dsA() + dsF(); });
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
        auto dsA = reaction::calc([](int aa) { return aa; }, a);
        dsA.setName("dsA");

        dsB.reset([&]() { return a() + dsA(); });
        dsC.reset([&]() { return a() + dsA() + dsB(); });
        dsD.reset([&]() { return dsA() + dsB() + dsC(); });
        dsE.reset([&]() { return dsB() * dsC() + dsD(); });
    }

    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);
    EXPECT_EQ(dsD.get(), 7);
    EXPECT_EQ(dsE.get(), 15);

    a.value(10);
    ;
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

        dsB.reset([&]() { return a() + dsA(); });
        dsC.reset([&]() { return a() + dsA() + dsB(); });
        dsD.reset([&]() { return dsA() + dsB() + dsC(); });
        dsE.reset([&]() { return dsB() + dsC() + dsD(); });
    }

    EXPECT_EQ(dsB.get(), 2);
    EXPECT_EQ(dsC.get(), 4);
    EXPECT_EQ(dsD.get(), 7);
    EXPECT_EQ(dsE.get(), 13);

    a.value(10);
    EXPECT_EQ(dsB.get(), 11);
    EXPECT_EQ(dsC.get(), 22);
    EXPECT_EQ(dsD.get(), 34);
    EXPECT_EQ(dsE.get(), 67);
}

TEST(ReactionTest, TestReactContainer) {
    using namespace reaction;
    constexpr int N = 10;
    ReactContain<VarExpr, int> rc;
    for (int i = 0; i < N; ++i) {
        rc.push_back(unique(i));
    }
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(rc[i]->get(), i);
    }

    ReactContain<CalcExpr, int, std::set> rc2;
    for (int i = 0; i < N; ++i) {
        rc2.insert(unique([i, &rc]() { return (*rc[i])(); }));
    }
    for (int i = 0; i < N; ++i) {
        rc[i]->value(i + 1);
    }
    int index = 0;
    for (auto &r : rc2) {
        EXPECT_EQ(r->get(), ++index);
    }

    ReactContain<CalcExpr, VoidWrapper, std::list> rc3;
    for (int i = 0; i < N; ++i) {
        rc3.push_back(unique([i, &rc]() { std::cout << " rc " << i << " changed to " << (*rc[i])() << '\n'; }));
    }
    for (int i = 0; i < N; ++i) {
        rc[i]->value(i - 1);
    }

    ReactMap<CalcExpr, int, std::string> rc4;
    for (int i = 0; i < N; ++i) {
        rc4.insert({i, unique([i, &rc]() { return std::to_string((*rc[i])()); })});
    }
    for (int i = 0; i < N; ++i) {
        rc[i]->value(i + 1);
    }
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(rc4[i]->get(), std::to_string(i + 1));
    }

    ReactMap<CalcExpr, int, std::string, std::unordered_map, ReactAsKey> rc5;
    for (int i = 0; i < N; ++i) {
        rc5.emplace(unique([i, &rc]() { return (*rc[i])(); }), std::to_string(i));
    }
    for (int i = 0; i < N; ++i) {
        rc[i]->value(i * 2);
    }
    for (auto &[key, value] : rc5) {
        EXPECT_EQ(key->get(), std::stoi(value) * 2);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
