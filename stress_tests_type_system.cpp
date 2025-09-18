/*
 * Type System and Template Stress Tests for Reaction Framework
 * 
 * These tests focus on template instantiation, type deduction,
 * concept constraints, and other type system related edge cases.
 */

#include "reaction/reaction.h"
#include "gtest/gtest.h"
#include <type_traits>
#include <concepts>
#include <vector>
#include <array>
#include <tuple>
#include <variant>
#include <optional>
#include <memory>
#include <functional>
#include <typeinfo>
#include <string>

using namespace reaction;

class TypeSystemStressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test with various fundamental types
TEST_F(TypeSystemStressTest, FundamentalTypes) {
    // Test all fundamental types
    auto bool_var = var(true);
    auto char_var = var('A');
    auto int8_var = var(static_cast<int8_t>(42));
    auto uint8_var = var(static_cast<uint8_t>(42));
    auto int16_var = var(static_cast<int16_t>(1000));
    auto uint16_var = var(static_cast<uint16_t>(1000));
    auto int32_var = var(static_cast<int32_t>(100000));
    auto uint32_var = var(static_cast<uint32_t>(100000));
    auto int64_var = var(static_cast<int64_t>(1000000000LL));
    auto uint64_var = var(static_cast<uint64_t>(1000000000ULL));
    auto float_var = var(3.14f);
    auto double_var = var(3.14159);
    auto long_double_var = var(3.14159L);
    
    // Test calculations with mixed types
    auto mixed_calc = calc([](bool b, char c, int8_t i8, uint8_t u8, 
                             int16_t i16, uint16_t u16, int32_t i32, uint32_t u32,
                             int64_t i64, uint64_t u64, float f, double d, long double ld) {
        return static_cast<double>(b) + static_cast<double>(c) + 
               static_cast<double>(i8) + static_cast<double>(u8) +
               static_cast<double>(i16) + static_cast<double>(u16) +
               static_cast<double>(i32) + static_cast<double>(u32) +
               static_cast<double>(i64) + static_cast<double>(u64) +
               static_cast<double>(f) + d + static_cast<double>(ld);
    }, bool_var, char_var, int8_var, uint8_var, int16_var, uint16_var,
       int32_var, uint32_var, int64_var, uint64_var, float_var, double_var, long_double_var);
    
    EXPECT_NO_THROW(mixed_calc.get());
    EXPECT_GT(mixed_calc.get(), 0.0);
}

// Test with complex template types
TEST_F(TypeSystemStressTest, ComplexTemplateTypes) {
    // Nested containers
    using NestedVector = std::vector<std::vector<std::vector<int>>>;
    NestedVector nested = {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
    auto nested_var = var(nested);
    
    auto flatten_calc = calc([](const NestedVector& nv) {
        std::vector<int> result;
        for (const auto& v1 : nv) {
            for (const auto& v2 : v1) {
                for (int val : v2) {
                    result.push_back(val);
                }
            }
        }
        return result;
    }, nested_var);
    
    auto flattened = flatten_calc.get();
    EXPECT_EQ(flattened.size(), 8);
    EXPECT_EQ(flattened[0], 1);
    EXPECT_EQ(flattened[7], 8);
    
    // Complex map types
    using ComplexMap = std::map<std::string, std::vector<std::pair<int, double>>>;
    ComplexMap complex_map;
    complex_map["key1"] = {{1, 1.1}, {2, 2.2}};
    complex_map["key2"] = {{3, 3.3}, {4, 4.4}};
    
    auto map_var = var(complex_map);
    auto map_calc = calc([](const ComplexMap& cm) {
        double sum = 0.0;
        for (const auto& [key, pairs] : cm) {
            for (const auto& [i, d] : pairs) {
                sum += static_cast<double>(i) + d;
            }
        }
        return sum;
    }, map_var);
    
    EXPECT_NEAR(map_calc.get(), 20.0, 0.001); // 1+1.1+2+2.2+3+3.3+4+4.4 = 21
}

// Test with function types and callable objects
TEST_F(TypeSystemStressTest, FunctionTypesAndCallables) {
    // Function pointers
    int (*add_func)(int, int) = [](int a, int b) { return a + b; };
    auto func_ptr_var = var(add_func);
    
    auto func_calc = calc([](int (*f)(int, int)) {
        return f(10, 20);
    }, func_ptr_var);
    
    EXPECT_EQ(func_calc.get(), 30);
    
    // std::function
    std::function<std::string(const std::string&)> string_func = [](const std::string& s) {
        return s + "_processed";
    };
    auto std_func_var = var(string_func);
    
    auto std_func_calc = calc([](const std::function<std::string(const std::string&)>& f) {
        return f("test");
    }, std_func_var);
    
    EXPECT_EQ(std_func_calc.get(), "test_processed");
    
    // Lambda with captures
    int capture_value = 42;
    auto lambda = [capture_value](int x) { return x + capture_value; };
    auto lambda_var = var(lambda);
    
    auto lambda_calc = calc([](const auto& l) {
        return l(8);
    }, lambda_var);
    
    EXPECT_EQ(lambda_calc.get(), 50);
}

// Test with smart pointers and RAII types
TEST_F(TypeSystemStressTest, SmartPointersAndRAII) {
    // unique_ptr
    auto unique_ptr_var = var(std::make_unique<std::string>("hello"));
    
    auto unique_calc = calc([](const std::unique_ptr<std::string>& ptr) {
        return ptr ? ptr->length() : 0;
    }, unique_ptr_var);
    
    EXPECT_EQ(unique_calc.get(), 5);
    
    unique_ptr_var.value(std::make_unique<std::string>("world!!!"));
    EXPECT_EQ(unique_calc.get(), 8);
    
    unique_ptr_var.value(nullptr);
    EXPECT_EQ(unique_calc.get(), 0);
    
    // shared_ptr
    auto shared_ptr_var = var(std::make_shared<std::vector<int>>(std::initializer_list<int>{1, 2, 3, 4, 5}));
    
    auto shared_calc = calc([](const std::shared_ptr<std::vector<int>>& ptr) {
        return ptr ? ptr->size() : 0;
    }, shared_ptr_var);
    
    EXPECT_EQ(shared_calc.get(), 5);
    
    // Test reference counting
    auto shared_copy = shared_ptr_var.get();
    EXPECT_EQ(shared_copy.use_count(), 2); // One in var, one in our copy
    
    shared_ptr_var.value(std::make_shared<std::vector<int>>(10, 42));
    EXPECT_EQ(shared_calc.get(), 10);
    EXPECT_EQ(shared_copy.use_count(), 1); // Only our copy remains
}

// Test with template template parameters and complex inheritance
TEST_F(TypeSystemStressTest, TemplateTemplateParameters) {
    // Container of containers
    template<template<typename> class Container>
    struct ContainerWrapper {
        Container<int> data;
        
        ContainerWrapper() = default;
        ContainerWrapper(std::initializer_list<int> init) : data(init) {}
        
        size_t size() const { return data.size(); }
        void add(int value) { 
            if constexpr (std::is_same_v<Container<int>, std::vector<int>>) {
                data.push_back(value);
            } else if constexpr (std::is_same_v<Container<int>, std::set<int>>) {
                data.insert(value);
            }
        }
        
        bool operator==(const ContainerWrapper& other) const {
            return data == other.data;
        }
    };
    
    auto vector_wrapper_var = var(ContainerWrapper<std::vector>{1, 2, 3});
    auto set_wrapper_var = var(ContainerWrapper<std::set>{1, 2, 3});
    
    auto vector_calc = calc([](const ContainerWrapper<std::vector>& wrapper) {
        return wrapper.size();
    }, vector_wrapper_var);
    
    auto set_calc = calc([](const ContainerWrapper<std::set>& wrapper) {
        return wrapper.size();
    }, set_wrapper_var);
    
    EXPECT_EQ(vector_calc.get(), 3);
    EXPECT_EQ(set_calc.get(), 3);
    
    // Modify wrappers
    auto vector_wrapper = vector_wrapper_var.get();
    vector_wrapper.add(4);
    vector_wrapper_var.value(vector_wrapper);
    
    auto set_wrapper = set_wrapper_var.get();
    set_wrapper.add(1); // Duplicate, shouldn't increase size
    set_wrapper_var.value(set_wrapper);
    
    EXPECT_EQ(vector_calc.get(), 4);
    EXPECT_EQ(set_calc.get(), 3); // Set ignores duplicates
}

// Test with CRTP and complex inheritance hierarchies
TEST_F(TypeSystemStressTest, CRTPAndInheritance) {
    // CRTP base
    template<typename Derived>
    class CRTPBase {
    public:
        auto process() const {
            return static_cast<const Derived*>(this)->processImpl();
        }
        
        bool operator==(const CRTPBase& other) const {
            return static_cast<const Derived*>(this)->equals(static_cast<const Derived&>(other));
        }
    };
    
    class ConcreteA : public CRTPBase<ConcreteA> {
    public:
        int value;
        ConcreteA(int v) : value(v) {}
        
        int processImpl() const { return value * 2; }
        bool equals(const ConcreteA& other) const { return value == other.value; }
    };
    
    class ConcreteB : public CRTPBase<ConcreteB> {
    public:
        std::string data;
        ConcreteB(const std::string& d) : data(d) {}
        
        std::string processImpl() const { return data + "_processed"; }
        bool equals(const ConcreteB& other) const { return data == other.data; }
    };
    
    auto crtp_a_var = var(ConcreteA(21));
    auto crtp_b_var = var(ConcreteB("test"));
    
    auto crtp_a_calc = calc([](const ConcreteA& obj) {
        return obj.process();
    }, crtp_a_var);
    
    auto crtp_b_calc = calc([](const ConcreteB& obj) {
        return obj.process();
    }, crtp_b_var);
    
    EXPECT_EQ(crtp_a_calc.get(), 42);
    EXPECT_EQ(crtp_b_calc.get(), "test_processed");
}

// Test with variadic templates and parameter packs
TEST_F(TypeSystemStressTest, VariadicTemplatesAndParameterPacks) {
    // Variadic sum function
    template<typename... Args>
    auto variadic_sum(Args... args) {
        return (args + ...); // C++17 fold expression
    }
    
    auto var1 = var(1);
    auto var2 = var(2.5);
    auto var3 = var(3);
    auto var4 = var(4.7f);
    
    auto variadic_calc = calc([](int a, double b, int c, float d) {
        return variadic_sum(a, b, c, d);
    }, var1, var2, var3, var4);
    
    EXPECT_NEAR(variadic_calc.get(), 11.2, 0.001);
    
    // Variadic tuple processing
    auto tuple_var = var(std::make_tuple(10, 20.5, std::string("hello"), 'X'));
    
    auto tuple_calc = calc([](const std::tuple<int, double, std::string, char>& t) {
        auto process_element = []<typename T>(const T& element) -> std::string {
            if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(element);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return element;
            } else if constexpr (std::is_same_v<T, char>) {
                return std::string(1, element);
            } else {
                return "unknown";
            }
        };
        
        return process_element(std::get<0>(t)) + "_" +
               process_element(std::get<1>(t)) + "_" +
               process_element(std::get<2>(t)) + "_" +
               process_element(std::get<3>(t));
    }, tuple_var);
    
    EXPECT_EQ(tuple_calc.get(), "10_20.500000_hello_X");
}

// Test with concepts and constraints (C++20)
TEST_F(TypeSystemStressTest, ConceptsAndConstraints) {
    // Custom concept
    template<typename T>
    concept Numeric = std::is_arithmetic_v<T>;
    
    template<typename T>
    concept Container = requires(T t) {
        t.size();
        t.begin();
        t.end();
    };
    
    // Function that only accepts numeric types
    auto numeric_processor = [](Numeric auto value) {
        return value * 2;
    };
    
    auto int_var = var(21);
    auto double_var = var(3.14);
    
    auto numeric_calc1 = calc([&numeric_processor](int x) {
        return numeric_processor(x);
    }, int_var);
    
    auto numeric_calc2 = calc([&numeric_processor](double x) {
        return numeric_processor(x);
    }, double_var);
    
    EXPECT_EQ(numeric_calc1.get(), 42);
    EXPECT_NEAR(numeric_calc2.get(), 6.28, 0.001);
    
    // Function that only accepts containers
    auto container_processor = [](const Container auto& container) {
        return container.size();
    };
    
    auto vector_var = var(std::vector<int>{1, 2, 3, 4, 5});
    auto string_var = var(std::string("hello world"));
    
    auto container_calc1 = calc([&container_processor](const std::vector<int>& v) {
        return container_processor(v);
    }, vector_var);
    
    auto container_calc2 = calc([&container_processor](const std::string& s) {
        return container_processor(s);
    }, string_var);
    
    EXPECT_EQ(container_calc1.get(), 5);
    EXPECT_EQ(container_calc2.get(), 11);
}

// Test with type erasure and any
TEST_F(TypeSystemStressTest, TypeErasureAndAny) {
    auto any_var = var<std::any>(42);
    
    auto any_processor = calc([](const std::any& a) -> std::string {
        try {
            if (a.type() == typeid(int)) {
                return "int: " + std::to_string(std::any_cast<int>(a));
            } else if (a.type() == typeid(double)) {
                return "double: " + std::to_string(std::any_cast<double>(a));
            } else if (a.type() == typeid(std::string)) {
                return "string: " + std::any_cast<std::string>(a);
            } else {
                return "unknown type";
            }
        } catch (const std::bad_any_cast& e) {
            return "bad cast: " + std::string(e.what());
        }
    }, any_var);
    
    EXPECT_EQ(any_processor.get(), "int: 42");
    
    any_var.value(std::any(3.14159));
    EXPECT_EQ(any_processor.get(), "double: 3.141590");
    
    any_var.value(std::any(std::string("hello")));
    EXPECT_EQ(any_processor.get(), "string: hello");
    
    // Test with custom type
    struct CustomType {
        int value;
        std::string name;
    };
    
    any_var.value(std::any(CustomType{100, "custom"}));
    EXPECT_EQ(any_processor.get(), "unknown type");
}

// Test with recursive template instantiation
TEST_F(TypeSystemStressTest, RecursiveTemplateInstantiation) {
    // Recursive data structure
    template<int N>
    struct RecursiveStruct {
        int value = N;
        RecursiveStruct<N-1> nested;
        
        int sum() const {
            return value + nested.sum();
        }
    };
    
    // Specialization to stop recursion
    template<>
    struct RecursiveStruct<0> {
        int value = 0;
        int sum() const { return value; }
    };
    
    auto recursive_var = var(RecursiveStruct<10>{});
    
    auto recursive_calc = calc([](const RecursiveStruct<10>& rs) {
        return rs.sum();
    }, recursive_var);
    
    // Sum should be 10+9+8+...+1+0 = 55
    EXPECT_EQ(recursive_calc.get(), 55);
}

// Test with template specializations
TEST_F(TypeSystemStressTest, TemplateSpecializations) {
    // Primary template
    template<typename T>
    struct TypeProcessor {
        static std::string process(const T& value) {
            return "generic: " + std::to_string(sizeof(T)) + " bytes";
        }
    };
    
    // Specializations
    template<>
    struct TypeProcessor<std::string> {
        static std::string process(const std::string& value) {
            return "string: " + value;
        }
    };
    
    template<>
    struct TypeProcessor<bool> {
        static std::string process(bool value) {
            return "bool: " + std::string(value ? "true" : "false");
        }
    };
    
    auto int_var = var(42);
    auto string_var = var(std::string("test"));
    auto bool_var = var(true);
    
    auto int_calc = calc([](int x) {
        return TypeProcessor<int>::process(x);
    }, int_var);
    
    auto string_calc = calc([](const std::string& s) {
        return TypeProcessor<std::string>::process(s);
    }, string_var);
    
    auto bool_calc = calc([](bool b) {
        return TypeProcessor<bool>::process(b);
    }, bool_var);
    
    EXPECT_EQ(int_calc.get(), "generic: 4 bytes");
    EXPECT_EQ(string_calc.get(), "string: test");
    EXPECT_EQ(bool_calc.get(), "bool: true");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}