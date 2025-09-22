/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test action functionality that responds to changes
TEST(AdvancedFeaturesTest, TestAction) {
    auto a  = reaction::var(1);
    auto b  = reaction::var(3.14);
    auto at = reaction::action(
        [](int aa, double bb) { std::cout << "a = " << aa << '\t' << "b = " << bb << '\t'; }, a, b);

    bool trigger = false;
    auto att     = reaction::action(
        [&]([[maybe_unused]] auto atat) {
            trigger = true;
            std::cout << "at trigger " << std::endl;
        },
        at);

    trigger = false;

    a.value(2);
    EXPECT_TRUE(trigger);
}

// Test field-based reactivity with objects
TEST(AdvancedFeaturesTest, TestField) {
    Person person{"lummy", 18};
    auto p = reaction::var(person);
    auto a = reaction::var(1);
    auto ds =
        reaction::calc([](int aa, auto pp) { return std::to_string(aa) + pp.getName(); }, a, p);

    EXPECT_EQ(ds.get(), "1lummy");
    p->setName("lummy-new");
    EXPECT_EQ(ds.get(), "1lummy-new");
}