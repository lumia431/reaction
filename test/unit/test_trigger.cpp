/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test different trigger (AlwaysTrig vs default)
TEST(TriggerTest, TestChangeTrig) {
    auto a            = reaction::var(1);
    auto b            = reaction::var(2);
    auto c            = reaction::var(3);
    int triggerCountA = 0;
    int triggerCountB = 0;
    auto ds           = reaction::calc<reaction::AlwaysTrig>(
        [&triggerCountA](int aa, double bb) {
            ++triggerCountA;
            return aa + bb;
        },
        a,
        b);
    auto dds = reaction::calc(
        [&triggerCountB](auto aa, auto cc) {
            ++triggerCountB;
            return aa + cc;
        },
        a,
        c);
    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(triggerCountB, 1);
    a.value(1);
    EXPECT_EQ(triggerCountA, 2);
    EXPECT_EQ(triggerCountB, 1);

    a.value(2);
    EXPECT_EQ(triggerCountA, 3);
    EXPECT_EQ(triggerCountB, 2);
}

// Test filter-based trigge
TEST(TriggerTest, TestFilterTrig) {
    auto a  = reaction::var(1);
    auto b  = reaction::var(2);
    auto c  = reaction::var(3);
    auto ds = reaction::calc([](int aa, double bb) { return aa + bb; }, a, b);
    auto dds =
        reaction::calc<reaction::FilterTrig>([](auto cc, auto dsds) { return cc + dsds; }, c, ds);
    a.value(2);
    EXPECT_EQ(ds.get(), 4);
    EXPECT_EQ(dds.get(), 7);

    dds.filter([&]() { return c() + ds() < 10; });
    a.value(3);
    EXPECT_EQ(dds.get(), 8);

    a.value(5);
    EXPECT_EQ(dds.get(), 8);
}