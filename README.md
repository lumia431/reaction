# ReactiveCpp Framework

A header-only reactive programming framework for modern C++ (C++20 required)

## Key Features

### 🚀 High-Performance Architecture
- Zero-cost abstractions using template metaprogramming
- Compile-time dependency graph optimization
- Minimal runtime overhead

### 🔗 Intelligent Dependency Management
- Automatic DAG detection and cycle prevention
- Fine-grained change propagation control
- Multi-level caching strategies

### 🛡️ Type Safety
- C++20 concept constraints
- Compile-time type checking
- Safe value semantics

### 🧩 Extensible Policies
| Policy Type      | Available Options               |
|------------------|---------------------------------|
| Trigger Policy   | ValueChange/Threshold/Timer/Custom |
| Invalidation     | Direct/KeepCalculate/LastValid  |

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 19.30+)
- CMake 3.15+

### Basic Usage

```cpp
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
            return temp + hum * 0.1;  // Simple temperature-humidity index calculation
        },
        temperature, humidity
    );
    
    // Create final display string
    auto displayStr = reaction::makeVariableDataSource(
        [](double thi) {
            return "Current THI: " + std::to_string(thi);
        },
        tempHumidityIndex
    );

    // Create final display action
    auto displayAction = reaction::makeActionSource(
        [](double thi) {
            std::cout << "Action Trigger, THI = " << thi << std::endl;
        },
        tempHumidityIndex);
    
    std::cout << displayStr.get() << std::endl;  // Initial value
    
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