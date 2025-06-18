# Reaction: Modern C++ Reactive Programming Framework

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Header-only](https://img.shields.io/badge/Header--only-Yes-green.svg)](https://en.wikipedia.org/wiki/Header-only)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blueviolet.svg)](https://cmake.org)
[![ReactiveX](https://img.shields.io/badge/Reactive-Programming-ff69b4.svg)](https://reactivex.io)
[![TMP](https://img.shields.io/badge/Template-Metaprogramming-orange.svg)](https://en.cppreference.com/w/cpp/language/templates)
[![MVVM](https://img.shields.io/badge/Pattern-MVVM%2FMVC-9cf.svg)](https://en.wikipedia.org/wiki/Model–view–viewmodel)

Reaction is a blazing-fast, modern C++20 header-only reactive framework that brings React/Vue-style dataflow to native C++ – perfect for **UI Dataflow**, **Game logic**, **Financial Services**, **Real-time calculation** and more.

## 🚀 Performance Optimized

- **Zero-cost abstractions** through compile-time calculation
- Minimal runtime overhead with **smart change propagation**
- Propagation efficiency **at the level of millions per second**

## 🔗 Intelligent Dependency Management

- Automatic **DAG detection** and cycle prevention
- Fine-grained **change propagation control**
- Optimize **multiple calls due to duplicate dependencies**

## 🛡️ Safety Guarantees

- Compile-time **type checking** with C++20 concepts
- Safe **value semantics** throughout the framework
- Framework manages node lifetime internally

---

## 🔍 Comparison: `QProperty` vs `RxCpp` vs `Reaction`

| Feature / Metric               | 🟩 **QProperty (Qt6)**                       | 🟨 **RxCpp**                                     | 🟥 **Reaction**                          |
| ------------------------------ | -------------------------------------------- | ------------------------------------------------ | ------------------------------------------------------------ |
| **Expression Support**         | ✅ `setBinding()`, but only **single-layer**  | ✅ Supports chained `map`, `combine_latest`, etc. | ✅✅ Fully supports **deep nested expressions**                |
| **Expression Nesting Depth**   | ❌ Limited to one layer                       | ⚠️ Supports nesting, but verbose                 | ✅ Unlimited depth with automatic dependency tracking         |
| **Update Propagation**         | Manual propagation per layer                 | Reactive push chain per layer                    | Automatic DAG-based propagation with pruning                 |
| **Dependency Tracking**        | ❌ Manual                                     | ⚠️ Manual via operator chaining                  | ✅ Automatic via lazy evaluation capturing dependencies       |
| **Performance (Update Delay)** | ✅ Fast (O(1) propagation)                    | ❌ Slow (heap allocations and nested chaining)    | ✅✅ Fast (pruned update, lazy eval, diffing)                  |
| **Memory Usage**               | ✅ Very low (stack + signals)                 | ❌ High (many heap-allocated observables)         | ⚠️ Moderate (DAG storage, optimized with small object opt.)  |
| **Syntax Simplicity**          | ✅ Simple (`setBinding`, `value()`)           | ❌ Verbose template syntax                        | ✅ Clean expression templates, close to natural syntax        |
| **Type Support**               | ✅ Built-ins and registered custom types      | ✅ Template-based, supports any type              | ✅ Type-erased or templated support for any combination       |
| **Container Support**          | ✅ Can be used in containers                  | ✅ Can compose multiple observables               | ✅ Supports container expressions (e.g. map/filter outputs)   |
| **Threading Model**            | UI-thread default, manual safety for signals | ✅ Multi-threaded pipelines                       | ✅ Main thread default, pluggable lock strategies             |
| **Error Handling**             | ❌ None                                       | ✅ Robust error flow (`on_error_resume_next`)     | ✅ Error node propagation, pluggable failure strategy         |
| **Debuggability**              | ⚠️ Lambdas harder to trace                   | ❌ Difficult due to complex types                 | ✅ Trackable dependencies, observable IDs, chain tracing      |
| **Template Instance Size**     | ✅ Small                                      | ❌ Huge (template explosion)                      | ✅ Optimized with type-erasure or instance deduplication      |
| **Build Time**                 | ✅ Fast                                       | ❌ Very slow for large expressions                | ✅ Separated headers, controllable instantiation              |
| **Learning Curve**             | ✅ Low (Qt-style usage)                       | ❌ Steep (functional style)                       | ⚠️ Medium (understanding type deduction + expression design) |
| **Use Case Fit**               | UI property binding, light state syncing     | Asynchronous pipelines, stream logic             | UI + state modeling + expression trees with complex logic    |

---

## 📊 Performance Benchmarks

Comparative performance results against rxcpp (tested on 2025-06-14):

### Deep Dependency Test (Tree Structure, Depth=10)

| Framework          | Avg. Time (ns) | Iterations   | Relative Speed |
|--------------------|---------------:|-------------:|---------------:|
| reaction       |          68.3  |    9,803,835 | **1,663x** faster |
| rxcpp              |     113,571    |        6,290 | (baseline)     |

### Wide Dependency Test (10,000 Nodes)

| Framework          | Avg. Time (ns) | Iterations   | Relative Speed |
|--------------------|---------------:|-------------:|---------------:|
| reaction       |     261,448    |        2,626 | **2.76x** faster |
| rxcpp              |     721,404    |          960 | (baseline)     |

### Key Findings:
1. **Deep dependency scenarios**: ~1,663x faster than rxcpp
2. **Wide dependency scenarios**: ~2.76x faster than rxcpp
3. **Test Environment**:
   - 8-core CPU @ 2.8GHz
   - 32KB L1 Data Cache
   - 4MB L2 Unified Cache
   - Linux 5.15.0-78-generic

## 📦 Requirements

- **Compiler**: C++20 compatible (GCC 10+, Clang 12+, MSVC 19.30+)
- **Build System**: CMake 3.15+

## 🛠 Installation

To build and install the `reaction` reactive framework, follow the steps below:

```bash
git clone https://github.com/lumia431/reaction.git && cd reaction
cmake -S . -B build
cmake --install build/ --prefix /your/install/path
```

After installation, you can include and link against reaction in your own CMake-based project:

```cmake
find_package(reaction REQUIRED)
```

If you want to run example or test units:

```bash
cmake -S . -DBUILD_EXAMPLES=TRUE -DBUILD_TESTS=TRUE -B build
cmake --build build/
```

## 🚀 Quick Start

```cpp
#include <reaction/reaction.h>
#include <iostream>
#include <iomanip>
#include <cmath>

int main() {
    using namespace reaction;

    // 1. Reactive variables for stock prices
    auto buyPrice = var(100.0).setName("buyPrice");      // Price at which stock was bought
    auto currentPrice = var(105.0);                      // Current market price

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
    currentPrice.value(110.0).value(95.0);  // Stock price increases
    buyPrice.value(90.0);                   // Buy price adjusted

    return 0;
}
```

## 📖 Basic Usage

### 1. Reactive Variables: `var`

Define reactive state variables with `var<T>`.

```cpp
auto a = reaction::var(1);         // int variable
auto b = reaction::var(3.14);      // double variable
```

- get value:

```cpp
auto val = a.get();
```

- assignment:

```cpp
a.value(2);
```

### 2. Derived Computation: calc

Use **calc** to create reactive computations based on one or more var instances.

- Lambda Capture Style:

```cpp
auto a = reaction::var(1);
auto b = reaction::var(3.14);
auto sum = reaction::calc([=]() {
    return a() + b();  // Retrieve current values using a() and b()
});
```

- Parameter Binding Style (High Performance):

```cpp
auto ds = reaction::calc([](auto aa, auto bb) {
    return std::to_string(aa) + std::to_string(bb);
}, a, b);  // Dependencies: a and b
```

### 3. Declarative Expression: expr

expr provides a clean and concise syntax to declare reactive expressions. The result automatically updates when any dependent variable changes.

```cpp
auto a = reaction::var(1);
auto b = reaction::var(2);
auto result = reaction::expr(a + b * 3);  // result updates automatically when 'a' or 'b' change
```

### 4. Reactive Side Effects: action

Register actions to perform side effects whenever the observed variables change.

```cpp
int val = 10;
auto a = reaction::var(1);
auto dds = reaction::action([&val]() {
    val = a();
});
```

Ofcourse, to get high performance can use Parameter Binding Style.

```cpp
int val = 10;
auto a = reaction::var(1);
auto dds = reaction::action([&val](auto aa) {
    val = aa;
}, a);
```

### 5. Reactive Struct Fields

For complex types with reactive fields allow you to define struct-like variables whose members are individually reactive.

Here's an example of a `PersonField` class:

```cpp
class PersonField : public reaction::FieldBase {
public:
    PersonField(std::string name, int age):
        m_name(reaction::field(name)),
        m_age(reaction::field(age)){}

    std::string getName() const { return m_name.get(); }
    void setName(const std::string &name) { m_name.value(name); }
    int getAge() const { return m_age.get(); }
    void setAge(int age) { m_age.value(age); }

private:
    reaction::Var<std::string> m_name;
    reaction::Var<int> m_age;
};

auto p = reaction::var(PersonField{"Jack", 18});
auto action = reaction::action(
    []() {
        std::cout << "Action Trigger , name = " << p().getName() << " age = " << p().getAge() << '\n';
    });

p->setName("Jackson"); // Action Trigger
p->setAge(28);         // Action Trigger
```

### 6. Copy and move semantics support

```cpp
auto a = reaction::var(1);
auto b = reaction::var(3.14);
auto ds = reaction::calc([]() { return a() + b(); });
auto ds_copy = ds;
auto ds_move = std::move(ds);
EXPECT_FALSE(static_cast<bool>(ds));
```

### 7. Resetting Nodes and Dependencies

The reaction framework allows you to **reset a computation node** by replacing its computation function.
This mechanism is useful when the result needs to be recalculated using a different logic or different dependencies after the node has been initially created.

``Note:`` **The return value type cannot be changed**

Below is an example that demonstrates the reset functionality:

```cpp
TEST(TestReset, ReactionTest) {
    auto a = reaction::var(1);
    auto b = reaction::var(std::string{"2"});
    auto ds = reaction::calc([]() { return std::to_string(a()); });
    ds.reset([=]() { return b() + "set"; });

    v.value("3");
    EXPECT_EQ(ds.get(), "3set");

    EXPECT_THROW(ds.reset([=]() { return a(); }), std::runtime_error);  // return type changed
    EXPECT_THROW(ds.reset([=]() { return ds(); }), std::runtime_error); // cycle dependent
}
```

### 8. Trigger Mode

The `reaction` framework supports various triggering mode to control when reactive computations are re-evaluated. This example demonstrates three mode:

1. **Value Change Trigger:** The reactive computation is triggered only when the underlying value actually changes.
2. **Filter Trigger:** The reactive computation is triggered when the value crosses a specified condition.
3. **Always Trigger:** (Not explicitly shown in this example) Always triggers regardless of whether the value has changed.

The trigger Mode can be specified by the type parameter

```cpp
using namespace reaction;
auto stockPrice = var(100.0);
auto profit = expr(stockPrice() - 100.0);   // defalut ChangeTrigger
auto assignAction = action<AlwaysTrig>([=]() {
    std::cout << "Checky assign, price = " << stockPrice() <<'\n';
});
auto sellAction = action<FilterTrig>([=]() {
    std::cout << "It's time to sell, profit = " << profit() <<'\n';
});
sellAction.filter([=]() {
    return profit() > 5.0;
});
stockPrice.value(100.0); // assignAction trigger
stockPrice.value(101.0); // assignAction, profit trigger
stockPrice.value(106.0); // all trigger

```

You can even define a trigger mode yourself in your code, just include the **checkTrig** method:

```cpp
struct MyTrig {
    bool checkTrig() {
        // do something
        return true;
    }
};
auto a = var(1);
auto b = expr<MyTrig>(a + 1);
```

### 9. Invalid Strategies

In the `reaction` framework, all data sources **obtained by users are actually in the form of weak references**, and their actual memory is managed **in the observer map**.
Users can manually call the **close** method, so that all dependent data sources will also be closed.

```cpp
auto a = reaction::var(1);
auto b = reaction::var(2);
auto dsA = reaction::calc([=]() { return a(); });
auto dsB = reaction::calc([=]() { return dsA() + b(); });
dsA.close(); //dsB will automatically close, cause dsB dependents dsA.
EXPECT_FALSE(static_cast<bool>(dsA));
EXPECT_FALSE(static_cast<bool>(dsB));
```

However, for scenarios where the lifecycle of a weak reference acquired by user ends, the `reaction` framework makes several strategy for different scenarios.

- **DirectCloseStrategy:**
  The node is immediately closed (made invalid) when any of its dependencies become invalid.

- **KeepCalcStrategy:**
  The node continues to recalculate, its dependencies work normally.

- **LastValStrategy:**
  The node retains the last valid, its dependencies use the value to calculate.

Below is a concise example that illustrates all three strategies:

```cpp
{
    auto a = var(1);
    auto b = calc([]() { return a(); });
    {
        auto temp = calc<AlwaysTrig, CloseStra>([]() { return a(); });
        b.set([](auto t) { return t; }, temp);
    }
    // temp lifecycle ends, b will end too.
    EXPECT_FALSE(static_cast<bool>(b));
}
{
    auto a = var(1);
    auto b = calc([]() { return a(); });
    {
        auto temp = calc<AlwaysTrig, KeepStra>([]() { return a(); }); // default is KeepStra
        b.set([](auto t) { return t; }, temp);
    }
    // temp lifecycle ends, b not be influenced.
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(b.get(), 1);
    a.value(2);
    EXPECT_EQ(b.get(), 2);
}
{
    auto a = var(1);
    auto b = calc([]() { return a(); });
    {
        auto temp = calc<AlwaysTrig, LastStra>([]() { return a(); });
        b.set([](auto t) { return t; }, temp);
    }
    // temp lifecycle ends, b use its last val to calculate.
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(b.get(), 1);
    a.value(2);
    EXPECT_EQ(b.get(), 1);
}
```

Likewise, you can define a strategy yourself in your code, just include the **handleInvalid** method:

```cpp
struct MyStra {
    void handleInvalid() {
        std::cout << "Invalid" << std::endl;
    }
};
auto a = var(1);
auto b = expr<AlwaysTrig, MyStra>(a + 1);
```

### 10. Reactive Containers

**Reaction** supports reactive versions of standard stl containers (`vector, list, set, map`, etc.).

```cpp

using namespace reaction;
constexpr int STUDENT_COUNT = 5;
// 1. Student grades container - using vector to store VarExpr
std::vector<Var<double>> grades;
for (int i = 0; i < STUDENT_COUNT; ++i) {
    grades.push_back(create(70.0 + i * 5));
}
// 2. Grade statistics container - using list to store CalcExpr
std::list<Calc<double>> stats;
stats.push_back(create([&] {
    double sum = 0;
    for (auto &grade : grades)
        sum += grade();
    return sum / grades.size();
}));
stats.push_back(create([&] {
    double max = grades[0].get();
    for (auto &grade : grades)
        max = std::max(max, grade());
    return max;
}));
// 3. Grade change monitors - using set to store Action
std::set<Action<>> monitors;
for (int i = 0; i < STUDENT_COUNT; ++i) {
    monitors.insert(create([i, &grades] {
        std::cout << "[Monitor] Student " << i << " grade updated: " << grades[i]() << "\n";
    }));
}
// 4. Grade level mapping - using map to store CalcExpr
std::map<int, Calc<const char *>> gradeLevels;
for (int i = 0; i < STUDENT_COUNT; ++i) {
    gradeLevels.insert({i, create([i, &grades] {
                            double g = grades[i]();
                            if (g >= 90) return "A";
                            if (g >= 80) return "B";
                            if (g >= 70) return "C";
                            return "D";
                        })});
}

```

## **Contributions Welcome!**

We welcome all forms of contributions to make **Reaction** even better:

### **How to Contribute**
1. **Report Issues**
   🐛 Found a bug? [Open an Issue](https://github.com/lumia431/reaction/issues) with detailed reproduction steps.

2. **Suggest Features**
   💡 Have an idea? Propose new features through GitHub Discussions.

3. **Submit Pull Requests**
   👩💻 Follow our workflow:
   ```bash
   git clone https://github.com/lumia431/reaction.git
   cd reaction
   # Create a feature branch (feat/xxx or fix/xxx)
   # Submit PR against `dev` branch