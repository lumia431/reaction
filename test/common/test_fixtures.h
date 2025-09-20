/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <reaction.h>
#include "gtest/gtest.h"

/**
 * @brief Person class demonstrating field-based reactivity.
 *
 * This class is used across multiple test files to demonstrate
 * how reactive fields work with objects.
 */
class Person : public reaction::FieldBase {
public:
    Person(std::string name, int age) : m_name(field(name)), m_age(field(age)) {
    }

    std::string getName() const {
        return m_name.get();
    }

    void setName(const std::string &name) {
        m_name.value(name);
    }

    int getAge() const {
        return m_age.get();
    }

    void setAge(int age) {
        m_age.value(age);
    }

private:
    reaction::Var<std::string> m_name;
    reaction::Var<int> m_age;
};

/**
 * @brief Base test fixture providing common setup for reaction tests.
 */
class ReactionTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup if needed
    }

    void TearDown() override {
        // Common cleanup if needed
    }
};