#pragma once

#include "reaction/observerNode.h"
#include <any>
#include <exception>
#include <functional>

namespace reaction {

inline thread_local std::function<void(NodePtr)> g_reg_fun = nullptr;

struct RegGuard {
    RegGuard(const std::function<void(NodePtr)> &f) {
        g_reg_fun = f;
    }
    ~RegGuard() {
        g_reg_fun = nullptr;
    }
};

class ValueTpErase {
public:
    ValueTpErase() = default;

    template <typename T>
        requires(!std::is_same_v<std::decay_t<T>, ValueTpErase>)
    ValueTpErase(T &&value) : m_value(std::forward<T>(value)) {}

    bool empty() const noexcept {
        return !m_value.has_value();
    }

    const std::type_info &type() const noexcept {
        return m_value.type();
    }

    template <typename T>
    T value() const {
        if (empty()) {
            throw std::runtime_error("Accessing empty ValueTpErase");
        }

        try {
            return std::any_cast<T>(m_value);
        } catch (const std::bad_any_cast &) {
            throw std::bad_cast();
        }
    }

    template <typename T>
    operator T() const {
        return value<T>();
    }

private:
    std::any m_value;
};

template <typename Expr, typename Type>
struct ReactBase {

    virtual ~ReactBase() = default;

    decltype(auto) get() {
        return getAny().template value<Type>();
    }

    decltype(auto) operator()() {
        return reg().template value<Type>();
    }

    Type *operator->() const {
        return arrow().template value<Type*>();;
    }

    virtual operator bool() const = 0;
    virtual ValueTpErase getAny() const = 0;
    virtual ValueTpErase reg() const = 0;
    virtual ValueTpErase arrow() const = 0;
    virtual ReactBase &reset(const std::function<ValueTpErase()> &fun) = 0;
    virtual ReactBase &value(ValueTpErase val) = 0;
    virtual ReactBase &filter(const std::function<bool()> &fun) = 0;
    virtual ReactBase &close() = 0;
    virtual ReactBase &setName(const std::string &name) = 0;
    virtual std::string getName() const = 0;
    virtual NodePtr getNodePtr() const = 0;

    bool operator<(const ReactBase &other) const {
        return getName() < other.getName();
    }
};

} // namespace reaction

namespace std {
template <typename Expr, typename Type>
struct hash<reaction::ReactBase<Expr, Type>> {
    std::size_t operator()(const reaction::ReactBase<Expr, Type> &react) const noexcept {
        return std::hash<reaction::ObserverNode *>{}(react.getNodePtr().get());
    }
};

struct ReactEqual {
    template <typename Expr, typename Type>
    bool operator()(const reaction::ReactBase<Expr, Type> &a, const reaction::ReactBase<Expr, Type> &b) const {
        return a.getNodePtr() == b.getNodePtr();
    }
};

} // namespace std