#include "reaction/dataSource.h"
#include <iostream>
#include <string>

/**
 * Basic example demonstrating fundamental operations of reactive data sources
 * 1. Creating data sources
 * 2. Building data dependencies
 * 3. Reacting to data changes
 */
void basicExample() {
    // Create primary data sources
    auto temperature = reaction::makeMetaDataSource(25.0);
    auto humidity = reaction::makeMetaDataSource(60.0);

    // Create computed data source (temperature + humidity)
    auto tempHumidityIndex = reaction::makeVariableDataSource(
        [](double temp, double hum) {
            return temp + hum * 0.1; // Simple temperature-humidity index calculation
        },
        temperature, humidity);

    // Create final display string
    auto displayStr = reaction::makeVariableDataSource(
        [](double thi) {
            return "Current THI: " + std::to_string(thi);
        },
        tempHumidityIndex);

    // Create final display action
    auto displayAction = reaction::makeActionSource(
        [](double thi) {
            std::cout << "Action Trigger, THI = " << thi << std::endl;
        },
        tempHumidityIndex);

    std::cout << displayStr.get() << std::endl; // Initial value

    // Update temperature data
    *temperature = 30.0;                        // Action Trigger
    std::cout << displayStr.get() << std::endl; // Automatically updated

    // Update humidity data
    *humidity = 70.0;                           // Action Trigger
    std::cout << displayStr.get() << std::endl; // Automatically updated
}

int main() {
    basicExample();
    return 0;
}