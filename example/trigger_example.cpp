/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "reaction/reaction.h"
#include <iostream>

/**
 * Trigger example demonstrating different triggering strategies
 * 1. Value change trigger
 * 2. Threshold trigger
 * 3. Always trigger
 */
void triggerExample() {
    // Create primary data source
    auto stockPrice = reaction::var(100.0);
    stockPrice.setName("Stock Price");

    // Value change trigger example
    int valueChangeCount = 0;
    auto valueChangeDS = reaction::calc<reaction::ChangeTrig>(
        [&valueChangeCount](double price) {
            valueChangeCount++;
            return price * 1.1; // Calculate 10% price increase
        },
        stockPrice);

    // Threshold trigger example
    int thresholdCount = 0;
    auto thresholdDS = reaction::calc<reaction::FilterTrig>(
        [&thresholdCount](double price) {
            thresholdCount++;
            return price > 105.0 ? "Sell" : "Hold";
        },
        stockPrice);
    thresholdDS.filter([&]() { return stockPrice() > 105.0 || stockPrice() < 95.0; });

    // Test trigger logic
    stockPrice.value(101.0); // Only triggers value change
    stockPrice.value(101.0); // Same value doesn't trigger
    stockPrice.value(106.0); // Triggers both value change and threshold

    std::cout << "Value change triggers: " << valueChangeCount << '\n';
    std::cout << "Threshold triggers: " << thresholdCount << '\n';
    std::cout << "Current recommendation: " << thresholdDS.get() << '\n';
}

int main() {
    triggerExample();
    return 0;
}