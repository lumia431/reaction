#pragma once

#include "reaction/observerNode.h"
#include <exception>

namespace reaction {

template <typename Type>
class Resource : public ObserverNode {
public:
    Resource() : m_ptr(nullptr) {
    }

    template <typename T>
    Resource(T &&t) : m_ptr(std::make_unique<Type>(std::forward<T>(t))) {
    }

    Resource(const Resource &) = delete;
    Resource &operator=(const Resource &) = delete;

    Type &getValue() const {
        if (!m_ptr) {
            throw std::runtime_error("Resource is not initialized");
        }
        return *m_ptr;
    }

    template <typename T>
    void updateValue(T &&t) {
        if (!m_ptr) {
            m_ptr = std::make_unique<Type>(std::forward<T>(t));
        } else {
            *m_ptr = std::forward<T>(t);
        }
    }

    Type *getRawPtr() const {
        if (!this->m_ptr) {
            throw std::runtime_error("Attempt to get a null pointer");
        }
        return this->m_ptr.get();
    }

protected:
    std::unique_ptr<Type> m_ptr;
};

struct VoidWrapper {};

template <>
class Resource<VoidWrapper> : public ObserverNode {
public:
    VoidWrapper getValue() const {
        return VoidWrapper{};
    }
};
} // namespace reaction
