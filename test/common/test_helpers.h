/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <reaction.h>

#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <set>
#include <vector>

/**
 * @brief Common includes and helper functions for reaction tests.
 */

namespace reaction_test {

/**
 * @brief Helper function to measure execution time.
 */
template <typename Func> auto measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

/**
 * @brief Helper to create a vector of reactive variables.
 */
template <typename T>
std::vector<reaction::Var<T>> createReactiveVector(int size, T initial_value = T{}) {
    std::vector<reaction::Var<T>> result;
    for (int i = 0; i < size; ++i) {
        result.push_back(reaction::create(initial_value));
    }
    return result;
}

} // namespace reaction_test