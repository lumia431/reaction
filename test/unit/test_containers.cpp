/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test reactive variables in various STL containers
TEST(ContainerSupportTest, TestReactContainer) {
    using namespace reaction;
    constexpr int N = 10;
    std::vector<Var<int>> rc;
    for (int i = 0; i < N; ++i) {
        rc.push_back(create(i));
    }
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(rc[i].get(), i);
    }

    std::set<Calc<int>> rc2;
    for (int i = 0; i < N; ++i) {
        rc2.insert(create([i, &rc]() { return rc[i](); }));
    }
    for (int i = 0; i < N; ++i) {
        rc[i].value(i + 1);
    }
    int index = 0;
    for (auto &r : rc2) {
        EXPECT_EQ(r.get(), ++index);
    }

    std::list<Action<>> rc3;
    for (int i = 0; i < N; ++i) {
        rc3.push_back(create([i, &rc]() { std::cout << " rc " << i << " changed to " << rc[i]() << '\n'; }));
    }
    for (int i = 0; i < N; ++i) {
        rc[i].value(i - 1);
    }

    std::map<int, Calc<std::string>> rc4;
    for (int i = 0; i < N; ++i) {
        rc4.insert({i, create([i, &rc]() { return std::to_string(rc[i]()); })});
    }
    for (int i = 0; i < N; ++i) {
        rc[i].value(i + 1);
    }
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(rc4[i].get(), std::to_string(i + 1));
    }

    std::unordered_map<Calc<int>, std::string> rc5;
    for (int i = 0; i < N; ++i) {
        rc5.emplace(create([i, &rc]() { return rc[i](); }), std::to_string(i));
    }
    for (int i = 0; i < N; ++i) {
        rc[i].value(i * 2);
    }
    for (auto &[key, value] : rc5) {
        EXPECT_EQ(key.get(), std::stoi(value) * 2);
    }
}