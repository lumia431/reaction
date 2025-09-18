/*
 * Complex Scenarios and Edge Cases Stress Tests for Reaction Framework
 * 
 * These tests focus on complex real-world scenarios, unusual combinations
 * of features, and edge cases that might not be covered by unit tests.
 */

#include "reaction/reaction.h"
#include "gtest/gtest.h"
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <random>
#include <algorithm>
#include <functional>
#include <variant>
#include <optional>
#include <any>
#include <typeinfo>
#include <sstream>

using namespace reaction;

class ComplexScenariosTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test dynamic dependency graph reconstruction
TEST_F(ComplexScenariosTest, DynamicDependencyReconstruction) {
    constexpr int PHASES = 10;
    constexpr int NODES_PER_PHASE = 100;
    
    std::vector<Var<int>> sources;
    std::vector<Calc<int>> calculations;
    
    for (int phase = 0; phase < PHASES; ++phase) {
        // Add new source nodes
        for (int i = 0; i < NODES_PER_PHASE; ++i) {
            sources.push_back(var(phase * NODES_PER_PHASE + i));
        }
        
        // Create calculations that depend on nodes from current and previous phases
        std::random_device rd;
        std::mt19937 gen(rd());
        
        for (int i = 0; i < NODES_PER_PHASE; ++i) {
            if (phase == 0) {
                // First phase: simple calculations
                calculations.push_back(calc([i](int x) { return x * 2; }, sources[i]));
            } else {
                // Later phases: complex interdependencies
                std::uniform_int_distribution<> source_dis(0, sources.size() - 1);
                std::uniform_int_distribution<> calc_dis(0, calculations.size() - 1);
                
                int src_idx = source_dis(gen);
                int calc_idx = calc_dis(gen);
                
                calculations.push_back(calc([](int a, int b) { 
                    return (a + b) % 1000; // Prevent overflow
                }, sources[src_idx], calculations[calc_idx]));
            }
        }
        
        // Randomly reset some existing calculations
        if (phase > 0) {
            std::uniform_int_distribution<> reset_dis(0, calculations.size() - NODES_PER_PHASE - 1);
            for (int i = 0; i < NODES_PER_PHASE / 4; ++i) {
                int reset_idx = reset_dis(gen);
                std::uniform_int_distribution<> new_src_dis(phase * NODES_PER_PHASE, sources.size() - 1);
                int new_src = new_src_dis(gen);
                
                EXPECT_NO_THROW({
                    calculations[reset_idx].reset([](int x) { return x + 1000; }, sources[new_src]);
                });
            }
        }
        
        // Verify all calculations are still valid
        for (size_t i = 0; i < calculations.size(); ++i) {
            EXPECT_TRUE(static_cast<bool>(calculations[i]));
            EXPECT_NO_THROW(calculations[i].get());
        }
    }
    
    // Final stress test: update all sources
    batchExecute([&]() {
        for (size_t i = 0; i < sources.size(); ++i) {
            sources[i].value(static_cast<int>(i) + 10000);
        }
    });
    
    // Verify all calculations updated
    for (const auto& calc : calculations) {
        EXPECT_NO_THROW(calc.get());
    }
}

// Test complex trigger mode combinations
TEST_F(ComplexScenariosTest, ComplexTriggerModeCombinations) {
    auto source = var(0);
    
    std::atomic<int> always_count{0};
    std::atomic<int> change_count{0};
    std::atomic<int> filter_count{0};
    
    // Different trigger modes observing the same source
    auto always_action = action<AlwaysTrig>([&always_count](int) {
        always_count++;
    }, source);
    
    auto change_action = action<ChangeTrig>([&change_count](int) {
        change_count++;
    }, source);
    
    auto filter_action = action<FilterTrig>([&filter_count](int) {
        filter_count++;
    }, source);
    
    // Set filter condition
    filter_action.filter([&]() { return source() > 5; });
    
    // Test sequence of updates
    source.value(0);  // same value
    EXPECT_EQ(always_count.load(), 1);  // AlwaysTrig fires
    EXPECT_EQ(change_count.load(), 0);  // ChangeTrig doesn't fire (same value)
    EXPECT_EQ(filter_count.load(), 0);  // Filter condition not met
    
    source.value(1);  // new value, but filter condition not met
    EXPECT_EQ(always_count.load(), 2);
    EXPECT_EQ(change_count.load(), 1);
    EXPECT_EQ(filter_count.load(), 0);
    
    source.value(10); // new value, filter condition met
    EXPECT_EQ(always_count.load(), 3);
    EXPECT_EQ(change_count.load(), 2);
    EXPECT_EQ(filter_count.load(), 1);
    
    source.value(10); // same value
    EXPECT_EQ(always_count.load(), 4);
    EXPECT_EQ(change_count.load(), 2); // No change
    EXPECT_EQ(filter_count.load(), 1); // No change, so filter not evaluated
    
    // Test in batch
    batchExecute([&]() {
        source.value(15);
        source.value(20);
        source.value(25);
    });
    
    // Only final value should trigger notifications
    EXPECT_EQ(always_count.load(), 5);
    EXPECT_EQ(change_count.load(), 3);
    EXPECT_EQ(filter_count.load(), 2);
}

// Test invalidation strategy combinations
TEST_F(ComplexScenariosTest, InvalidationStrategyCombinations) {
    std::vector<Calc<int, CloseStra>> close_nodes;
    std::vector<Calc<int, KeepStra>> keep_nodes;
    std::vector<Calc<int, LastStra>> last_nodes;
    
    {
        auto temp1 = calc<ChangeTrig, CloseStra>([](){ return 1; });
        auto temp2 = calc<ChangeTrig, KeepStra>([](){ return 2; });
        auto temp3 = calc<ChangeTrig, LastStra>([](){ return 3; });
        
        // Create nodes with different strategies depending on temp nodes
        for (int i = 0; i < 10; ++i) {
            close_nodes.push_back(calc<ChangeTrig, CloseStra>([i](int x) { return x + i; }, temp1));
            keep_nodes.push_back(calc<ChangeTrig, KeepStra>([i](int x) { return x + i; }, temp2));
            last_nodes.push_back(calc<ChangeTrig, LastStra>([i](int x) { return x + i; }, temp3));
        }
        
        // All should be valid initially
        for (int i = 0; i < 10; ++i) {
            EXPECT_TRUE(static_cast<bool>(close_nodes[i]));
            EXPECT_TRUE(static_cast<bool>(keep_nodes[i]));
            EXPECT_TRUE(static_cast<bool>(last_nodes[i]));
            EXPECT_EQ(close_nodes[i].get(), 1 + i);
            EXPECT_EQ(keep_nodes[i].get(), 2 + i);
            EXPECT_EQ(last_nodes[i].get(), 3 + i);
        }
        
        // temp nodes go out of scope here
    }
    
    // Check strategy behavior
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(static_cast<bool>(close_nodes[i])); // Should be closed
        EXPECT_TRUE(static_cast<bool>(keep_nodes[i]));   // Should remain valid
        EXPECT_TRUE(static_cast<bool>(last_nodes[i]));   // Should remain valid
        
        EXPECT_EQ(keep_nodes[i].get(), 2 + i);  // Should continue working
        EXPECT_EQ(last_nodes[i].get(), 3 + i);  // Should use last value
    }
    
    // Test mixed dependencies
    auto mixed_calc = calc([&]() {
        int sum = 0;
        for (const auto& node : keep_nodes) {
            sum += node();
        }
        for (const auto& node : last_nodes) {
            sum += node();
        }
        return sum;
    });
    
    EXPECT_NO_THROW(mixed_calc.get());
}

// Test complex container interactions
TEST_F(ComplexScenariosTest, ComplexContainerInteractions) {
    // Test reactive containers with complex element types
    struct ComplexElement {
        int id;
        std::string name;
        std::vector<double> values;
        std::map<std::string, int> properties;
        
        bool operator==(const ComplexElement& other) const {
            return id == other.id && name == other.name && 
                   values == other.values && properties == other.properties;
        }
    };
    
    std::vector<Var<ComplexElement>> elements;
    
    // Create complex elements
    for (int i = 0; i < 100; ++i) {
        ComplexElement elem;
        elem.id = i;
        elem.name = "element_" + std::to_string(i);
        elem.values = {static_cast<double>(i), static_cast<double>(i * 2), static_cast<double>(i * 3)};
        elem.properties = {{"type", i % 3}, {"priority", i % 5}, {"group", i % 7}};
        
        elements.push_back(var(elem));
    }
    
    // Create aggregator calculations
    auto total_values = calc([&elements]() {
        double total = 0.0;
        for (const auto& elem : elements) {
            for (double val : elem().values) {
                total += val;
            }
        }
        return total;
    });
    
    auto property_counts = calc([&elements]() {
        std::map<int, int> counts;
        for (const auto& elem : elements) {
            int type = elem().properties.at("type");
            counts[type]++;
        }
        return counts;
    });
    
    // Verify initial calculations
    double expected_total = 0.0;
    for (int i = 0; i < 100; ++i) {
        expected_total += i + i * 2 + i * 3; // Sum of values for element i
    }
    EXPECT_NEAR(total_values.get(), expected_total, 0.001);
    
    auto counts = property_counts.get();
    EXPECT_EQ(counts.size(), 3);
    
    // Modify elements in complex ways
    batchExecute([&]() {
        for (int i = 0; i < 100; i += 10) {
            ComplexElement modified = elements[i].get();
            modified.values.push_back(static_cast<double>(i * 10));
            modified.properties["new_prop"] = i;
            elements[i].value(modified);
        }
    });
    
    // Verify calculations updated
    EXPECT_GT(total_values.get(), expected_total);
    EXPECT_NO_THROW(property_counts.get());
}

// Test variant and optional types
TEST_F(ComplexScenariosTest, VariantAndOptionalTypes) {
    using VariantType = std::variant<int, double, std::string, std::vector<int>>;
    
    auto variant_var = var<VariantType>(42);
    
    auto variant_processor = calc([](const VariantType& v) -> std::string {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int>) {
                return "int: " + std::to_string(value);
            } else if constexpr (std::is_same_v<T, double>) {
                return "double: " + std::to_string(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "string: " + value;
            } else if constexpr (std::is_same_v<T, std::vector<int>>) {
                return "vector size: " + std::to_string(value.size());
            }
            return "unknown";
        }, v);
    }, variant_var);
    
    EXPECT_EQ(variant_processor.get(), "int: 42");
    
    // Change variant type
    variant_var.value(3.14);
    EXPECT_EQ(variant_processor.get(), "double: 3.140000");
    
    variant_var.value(std::string("hello"));
    EXPECT_EQ(variant_processor.get(), "string: hello");
    
    variant_var.value(std::vector<int>{1, 2, 3, 4, 5});
    EXPECT_EQ(variant_processor.get(), "vector size: 5");
    
    // Test with optional
    auto optional_var = var<std::optional<int>>(std::nullopt);
    
    auto optional_processor = calc([](const std::optional<int>& opt) -> int {
        return opt.has_value() ? opt.value() * 2 : -1;
    }, optional_var);
    
    EXPECT_EQ(optional_processor.get(), -1);
    
    optional_var.value(21);
    EXPECT_EQ(optional_processor.get(), 42);
    
    optional_var.value(std::nullopt);
    EXPECT_EQ(optional_processor.get(), -1);
}

// Test function object and lambda combinations
TEST_F(ComplexScenariosTest, FunctionObjectAndLambdaCombinations) {
    // Custom function objects
    struct Multiplier {
        int factor;
        Multiplier(int f) : factor(f) {}
        int operator()(int x) const { return x * factor; }
    };
    
    struct Accumulator {
        mutable int sum = 0;
        int operator()(int x) const { sum += x; return sum; }
    };
    
    auto source = var(1);
    
    // Use function objects
    auto mult_calc = calc(Multiplier(5), source);
    auto accum_calc = calc(Accumulator(), source);
    
    EXPECT_EQ(mult_calc.get(), 5);
    EXPECT_EQ(accum_calc.get(), 1);
    
    source.value(3);
    EXPECT_EQ(mult_calc.get(), 15);
    EXPECT_EQ(accum_calc.get(), 4); // 1 + 3
    
    source.value(7);
    EXPECT_EQ(mult_calc.get(), 35);
    EXPECT_EQ(accum_calc.get(), 11); // 1 + 3 + 7
    
    // Test with std::function
    std::function<int(int)> func = [](int x) { return x * x; };
    auto func_calc = calc(func, source);
    EXPECT_EQ(func_calc.get(), 49);
    
    // Change the function
    func = [](int x) { return x + 100; };
    func_calc.reset(func, source);
    EXPECT_EQ(func_calc.get(), 107);
}

// Test recursive data structures
TEST_F(ComplexScenariosTest, RecursiveDataStructures) {
    struct TreeNode {
        int value;
        std::vector<std::shared_ptr<TreeNode>> children;
        
        TreeNode(int v) : value(v) {}
        
        void addChild(std::shared_ptr<TreeNode> child) {
            children.push_back(child);
        }
        
        int sum() const {
            int total = value;
            for (const auto& child : children) {
                total += child->sum();
            }
            return total;
        }
        
        int nodeCount() const {
            int count = 1;
            for (const auto& child : children) {
                count += child->nodeCount();
            }
            return count;
        }
    };
    
    // Create tree structure
    auto root = std::make_shared<TreeNode>(1);
    auto left = std::make_shared<TreeNode>(2);
    auto right = std::make_shared<TreeNode>(3);
    auto left_left = std::make_shared<TreeNode>(4);
    auto left_right = std::make_shared<TreeNode>(5);
    
    root->addChild(left);
    root->addChild(right);
    left->addChild(left_left);
    left->addChild(left_right);
    
    auto tree_var = var(root);
    
    auto sum_calc = calc([](const std::shared_ptr<TreeNode>& tree) {
        return tree->sum();
    }, tree_var);
    
    auto count_calc = calc([](const std::shared_ptr<TreeNode>& tree) {
        return tree->nodeCount();
    }, tree_var);
    
    EXPECT_EQ(sum_calc.get(), 15); // 1+2+3+4+5
    EXPECT_EQ(count_calc.get(), 5);
    
    // Modify tree structure
    auto new_root = std::make_shared<TreeNode>(10);
    new_root->addChild(root);
    new_root->addChild(std::make_shared<TreeNode>(20));
    
    tree_var.value(new_root);
    
    EXPECT_EQ(sum_calc.get(), 45); // 10+15+20
    EXPECT_EQ(count_calc.get(), 7);
}

// Test state machine simulation
TEST_F(ComplexScenariosTest, StateMachineSimulation) {
    enum class State { Idle, Processing, Completed, Error };
    
    struct StateMachine {
        State current_state = State::Idle;
        int process_count = 0;
        std::string last_error;
        
        void process() {
            if (current_state == State::Idle) {
                current_state = State::Processing;
            } else if (current_state == State::Processing) {
                process_count++;
                if (process_count >= 3) {
                    current_state = State::Completed;
                } else if (process_count == 2) {
                    // Simulate error condition
                    current_state = State::Error;
                    last_error = "Processing error at count 2";
                }
            }
        }
        
        void reset() {
            current_state = State::Idle;
            process_count = 0;
            last_error.clear();
        }
        
        bool operator==(const StateMachine& other) const {
            return current_state == other.current_state &&
                   process_count == other.process_count &&
                   last_error == other.last_error;
        }
    };
    
    auto state_machine = var(StateMachine{});
    
    auto state_monitor = calc([](const StateMachine& sm) -> std::string {
        switch (sm.current_state) {
            case State::Idle: return "Idle";
            case State::Processing: return "Processing (" + std::to_string(sm.process_count) + ")";
            case State::Completed: return "Completed";
            case State::Error: return "Error: " + sm.last_error;
        }
        return "Unknown";
    }, state_machine);
    
    auto can_process = calc([](const StateMachine& sm) {
        return sm.current_state == State::Idle || sm.current_state == State::Processing;
    }, state_machine);
    
    // Initial state
    EXPECT_EQ(state_monitor.get(), "Idle");
    EXPECT_TRUE(can_process.get());
    
    // Process once
    StateMachine sm = state_machine.get();
    sm.process();
    state_machine.value(sm);
    
    EXPECT_EQ(state_monitor.get(), "Processing (0)");
    EXPECT_TRUE(can_process.get());
    
    // Process again
    sm = state_machine.get();
    sm.process();
    state_machine.value(sm);
    
    EXPECT_EQ(state_monitor.get(), "Processing (1)");
    EXPECT_TRUE(can_process.get());
    
    // Process third time (triggers error)
    sm = state_machine.get();
    sm.process();
    state_machine.value(sm);
    
    EXPECT_EQ(state_monitor.get(), "Error: Processing error at count 2");
    EXPECT_FALSE(can_process.get());
    
    // Reset
    sm = state_machine.get();
    sm.reset();
    state_machine.value(sm);
    
    EXPECT_EQ(state_monitor.get(), "Idle");
    EXPECT_TRUE(can_process.get());
}

// Test event-driven patterns
TEST_F(ComplexScenariosTest, EventDrivenPatterns) {
    struct Event {
        std::string type;
        std::map<std::string, std::string> data;
        std::chrono::system_clock::time_point timestamp;
        
        Event(const std::string& t) : type(t), timestamp(std::chrono::system_clock::now()) {}
        
        bool operator==(const Event& other) const {
            return type == other.type && data == other.data;
        }
    };
    
    auto event_stream = var<std::optional<Event>>(std::nullopt);
    
    // Event counters
    std::map<std::string, int> event_counts;
    auto counter_calc = calc([&event_counts](const std::optional<Event>& event) {
        if (event.has_value()) {
            event_counts[event->type]++;
        }
        return event_counts;
    }, event_stream);
    
    // Event filter for specific types
    auto user_events = calc([](const std::optional<Event>& event) -> bool {
        return event.has_value() && event->type.starts_with("user.");
    }, event_stream);
    
    auto system_events = calc([](const std::optional<Event>& event) -> bool {
        return event.has_value() && event->type.starts_with("system.");
    }, event_stream);
    
    // Send various events
    Event login_event("user.login");
    login_event.data["user_id"] = "123";
    event_stream.value(login_event);
    
    auto counts = counter_calc.get();
    EXPECT_EQ(counts["user.login"], 1);
    EXPECT_TRUE(user_events.get());
    EXPECT_FALSE(system_events.get());
    
    Event system_start("system.start");
    event_stream.value(system_start);
    
    counts = counter_calc.get();
    EXPECT_EQ(counts["system.start"], 1);
    EXPECT_FALSE(user_events.get());
    EXPECT_TRUE(system_events.get());
    
    // Send multiple events of same type
    for (int i = 0; i < 5; ++i) {
        Event click_event("user.click");
        click_event.data["button"] = "button_" + std::to_string(i);
        event_stream.value(click_event);
    }
    
    counts = counter_calc.get();
    EXPECT_EQ(counts["user.click"], 5);
    EXPECT_TRUE(user_events.get());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}