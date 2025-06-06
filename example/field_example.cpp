/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "reaction/reaction.h"
#include <iostream>
#include <string>

// Person class with reactive fields for name and age
class Person : public reaction::FieldBase {
public:
    // Constructor initializing person data
    Person(const std::string &name, int age) :
        m_name(field(name)),
        m_age(field(age)) {
    }

    // Getter for name
    std::string getName() const {
        return m_name.get();
    }

    // Setter for name
    void setName(const std::string &name) {
        m_name.value(name);
    }

    // Getter for age
    int getAge() const {
        return m_age.get();
    }

    // Setter for age
    void setAge(int age) {
        m_age.value(age);
    }

    // Method to get person info as string
    std::string getInfo() const {
        return getName() + ", " + std::to_string(getAge()) + " years old";
    }

private:
    reaction::Field<std::string> m_name;
    reaction::Field<int> m_age;
};

/**
 * Example demonstrating reactive fields with Person class
 * 1. Creating reactive objects
 * 2. Building computed properties
 * 3. Reacting to field changes
 */
void personFieldExample() {
    // Create a reactive person instance
    auto person1 = reaction::var(Person{"Alice", 30});
    auto person2 = reaction::var(Person{"Jack", 20});

    // Create a computed greeting message
    auto ds = reaction::calc(
        [](const auto &p1, const auto &p2) {
            std::cout << "Person1 : " << p1.getInfo() << " Person2 : " << p2.getInfo() << '\n';
            return true;
        },
        person1, person2);
    // Update person's name and age
    person1->setName("Alice Johnson");
    person1->setAge(37);

    person2->setName("Jack Jones");
    person2->setAge(27);
}

int main() {
    personFieldExample();
    return 0;
}