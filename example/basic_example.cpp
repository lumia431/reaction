/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

/**
 * @file basic_example.cpp
 * @brief Basic usage example of the Reaction framework
 *
 * This example demonstrates:
 * - Creating reactive variables with var()
 * - Computing derived values with calc() and expr()
 * - Setting up reactive actions that respond to changes
 * - Basic reactive dataflow patterns
 *
 * Use case: Stock price tracking with automatic profit/loss calculation
 */

#include <reaction.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
    using namespace reaction;

    // === Step 1: Create reactive variables for stock prices ===
    auto buyPrice     = var(100.0).setName("buyPrice"); // Price at which stock was bought
    auto currentPrice = var(105.0);                     // Current market price

    // === Step 2: Compute profit/loss amount using calc() ===
    // calc() creates a reactive calculation that automatically updates
    // when its dependencies (buyPrice, currentPrice) change
    auto profit = calc([&]() { return currentPrice() - buyPrice(); });

    // === Step 3: Compute percentage gain/loss using expr() ===
    // expr() creates a reactive expression from operator overloads
    // This demonstrates the mathematical expression syntax
    auto profitPercent = expr(std::abs(currentPrice - buyPrice) / buyPrice * 100);

    // === Step 4: Set up reactive logging with action() ===
    // action() creates a side-effect that triggers when dependencies change
    // This will automatically print updates whenever prices change
    auto logger = action([&]() {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "[Stock Update] Current Price: $" << currentPrice() << ", Profit: $"
                  << profit() << " (" << profitPercent() << "%)\n";
    });

    // === Step 5: Simulate price changes ===
    // Watch how all dependent calculations update automatically
    currentPrice.value(110.0).value(95.0); // Stock price changes (chained updates)
    buyPrice.value(90.0);                  // Buy price adjusted

    return 0;
}