#include "reaction/dataSource.h"
#include <iostream>

/**
 * Trigger example demonstrating different triggering strategies
 * 1. Value change trigger
 * 2. Threshold trigger
 * 3. Always trigger
 */
void triggerExample() {
    // Create primary data source
    auto stockPrice = reaction::makeMetaDataSource(100.0);
    stockPrice.setName("Stock Price");

    // Value change trigger example
    int valueChangeCount = 0;
    auto valueChangeDS = reaction::makeVariableDataSource<reaction::ValueChangeTrigger>(
        [&valueChangeCount](double price) {
            valueChangeCount++;
            return price * 1.1; // Calculate 10% price increase
        },
        stockPrice);

    // Threshold trigger example
    int thresholdCount = 0;
    auto thresholdDS = reaction::makeVariableDataSource<reaction::ThresholdTrigger>(
        [&thresholdCount](double price) {
            thresholdCount++;
            return price > 105.0 ? "Sell" : "Hold";
        },
        stockPrice);
    thresholdDS.setThreshold([](double price) { return price > 105.0 || price < 95.0; }, stockPrice);

    // Test trigger logic
    *stockPrice = 101.0; // Only triggers value change
    *stockPrice = 101.0; // Same value doesn't trigger
    *stockPrice = 106.0; // Triggers both value change and threshold

    std::cout << "Value change triggers: " << valueChangeCount << std::endl;
    std::cout << "Threshold triggers: " << thresholdCount << std::endl;
    std::cout << "Current recommendation: " << thresholdDS.get() << std::endl;
}

int main() {
    triggerExample();
    return 0;
}