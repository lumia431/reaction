/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include <reaction/reaction.h>
#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
    using namespace reaction;

    // 1. Reactive variables for stock prices
    auto buyPrice = var(100.0).setName("buyPrice"); // Price at which stock was bought
    auto currentPrice = var(105.0);                 // Current market price

    // 2. Use 'calc' to compute profit or loss amount
    auto profit = calc([&]() {
        return currentPrice() - buyPrice();
    });

    // 3. Use 'expr' to compute percentage gain/loss
    auto profitPercent = expr(std::abs(currentPrice - buyPrice) / buyPrice * 100);

    // 4. Use 'action' to print the log whenever values change
    auto logger = action([&]() {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "[Stock Update] Current Price: $" << currentPrice()
                  << ", Profit: $" << profit()
                  << " (" << profitPercent() << "%)\n";
    });

    // Simulate price changes
    currentPrice.value(110.0).value(95.0); // Stock price increases
    buyPrice.value(90.0);                  // Buy price adjusted

    return 0;
}