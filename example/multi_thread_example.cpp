/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <reaction/reaction.h>
#include <thread>
#include <vector>

using namespace std;
using namespace reaction;

constexpr int N = 100;

thread_local std::mt19937 gen(std::random_device{}());
thread_local std::uniform_int_distribution<> dist(0, N - 1);

vector<Var<int>> g_v(N);
vector<Calc<int>> g_c(N);
vector<Action<>> g_a(N);

void test1() {
    auto rd1 = dist(gen);
    auto rd2 = dist(gen);
    auto rd3 = dist(gen);

    g_c[rd1].reset([&]() {
        return g_v[rd2]() + g_v[rd3]();
    });
}

void test2() {
    auto rd1 = dist(gen);
    auto rd2 = dist(gen);
    auto rd3 = dist(gen);

    g_a[rd1].reset([&]() {
        cout << g_c[rd2]() + g_c[rd3]() << '\n';
    });
}

int main() {
    for (int i = 0; i < N; ++i) {
        g_v[i] = var(i);
        g_c[i] = calc([]() { return 1; });
        g_a[i] = action([]() {});
    }

    vector<thread> v_t1, v_t2;
    v_t1.reserve(N);
    v_t2.reserve(N);

    for (int i = 0; i < N; ++i) {
        v_t1.emplace_back(test1);
        v_t2.emplace_back(test2);
    }

    for (auto &t : v_t1)
        t.join();
    for (auto &t : v_t2)
        t.join();

    cout << "Done\n";
    return 0;
}