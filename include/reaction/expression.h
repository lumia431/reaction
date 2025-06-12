/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include "reaction/resource.h"
#include "reaction/triggerMode.h"

namespace reaction {

/**
 * @brief Marker type for variable (mutable) expressions.
 */
struct VarExpr {};

/**
 * @brief Marker type for calculated (derived) expressions.
 */
struct CalcExpr {};

/**
 * @brief Binary operator wrapper for reactive expression trees.
 *
 * Encapsulates left and right operands with an operator to form an evaluatable expression.
 *
 * @tparam Op The binary operation (e.g., AddOp).
 * @tparam L  Left-hand side expression type.
 * @tparam R  Right-hand side expression type.
 */
template <typename Op, typename L, typename R>
class BinaryOpExpr {
public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;

    template <typename Left, typename Right>
    BinaryOpExpr(Left &&l, Right &&r, Op o = Op{})
        : left(std::forward<Left>(l)), right(std::forward<Right>(r)), op(o) {}

    /// @brief Evaluates the expression.
    auto operator()() const {
        return calculate();
    }

    /// @brief Implicit conversion to value type (evaluates expression).
    operator ValueType() {
        return calculate();
    }

private:
    auto calculate() const {
        return op(left(), right());
    }

    L left;
    R right;
    [[no_unique_address]] Op op;
};

// === Operator Functors ===

struct AddOp {
    auto operator()(auto &&l, auto &&r) const {
        return l + r;
    }
};

struct MulOp {
    auto operator()(auto &&l, auto &&r) const {
        return l * r;
    }
};

struct SubOp {
    auto operator()(auto &&l, auto &&r) const {
        return l - r;
    }
};

struct DivOp {
    auto operator()(auto &&l, auto &&r) const {
        return l / r;
    }
};

/**
 * @brief Wraps a literal value into a callable expression.
 *
 * @tparam T Value type.
 */
template <typename T>
struct ValueWrapper {
    using ValueType = T;
    T value;

    template <typename Type>
    ValueWrapper(Type &&t) : value(std::forward<Type>(t)) {}

    const T &operator()() const {
        return value;
    }
};

/**
 * @brief Creates a binary expression from two operands and an operator.
 */
template <typename Op, typename L, typename R>
auto make_binary_expr(L &&l, R &&r) {
    return BinaryOpExpr<Op, ExprTraits<std::decay_t<L>>, ExprTraits<std::decay_t<R>>>(
        std::forward<L>(l), std::forward<R>(r));
}

// === Operator Overloads for Reactive Expressions ===

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator+(L &&l, R &&r) {
    return make_binary_expr<AddOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator*(L &&l, R &&r) {
    return make_binary_expr<MulOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator-(L &&l, R &&r) {
    return make_binary_expr<SubOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasReactOp<L, R>
auto operator/(L &&l, R &&r) {
    return make_binary_expr<DivOp>(std::forward<L>(l), std::forward<R>(r));
}

/**
 * @brief Core reactive computation logic shared by all calculated expressions.
 *
 * Handles dependency registration, invalidation, and value recomputation.
 *
 * @tparam Type Computed value type.
 * @tparam TM   Triggering mode.
 */
template <typename Type, IsTrigMode TM>
class CalcExprBase : public Resource<Type>, public TM {
public:
    /**
     * @brief Sets the function source and its dependencies.
     */
    template <typename F, typename... A>
    void setSource(F &&f, A &&...args) {
        if constexpr (std::convertible_to<ReturnType<F, A...>, Type>) {
            this->updateObservers(args.getPtr()...);
            setFunctor(createFun(std::forward<F>(f), std::forward<A>(args)...));

            if constexpr (!VoidType<Type>) {
                this->updateValue(evaluate());
            } else {
                evaluate();
            }
        } else {
            throw std::runtime_error("return type can reset another!");
        }
    }

    /// @brief Registers an observer for dependency tracking.
    void addObCb(NodePtr node) {
        this->addOneObserver(node);
    }

private:
    /**
     * @brief Captures and wraps a function with weak references to arguments.
     */
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        return [f = std::forward<F>(f), ... args = args.getWeak()]() {
            if constexpr (VoidType<Type>) {
                std::invoke(f, args.lock()->get()...);
                return VoidWrapper{};
            } else {
                return std::invoke(f, args.lock()->get()...);
            }
        };
    }

    /// @brief Handles value change notifications and trigger checks.
    void valueChanged(bool changed) override {
        if constexpr (IsChangeTrig<TM>) {
            this->setChanged(changed);
        }

        if (TM::checkTrig()) {
            if constexpr (!VoidType<Type>) {
                auto oldVal = this->getValue();
                auto newVal = evaluate();
                this->updateValue(newVal);

                if constexpr (ComparableType<Type>) {
                    this->notify(oldVal != newVal);
                } else {
                    this->notify(true);
                }
            } else {
                evaluate();
                this->notify(true);
            }
        }
    }

    /// @brief Evaluates the current expression.
    auto evaluate() const {
        if constexpr (VoidType<Type>) {
            std::invoke(m_fun);
        } else {
            return std::invoke(m_fun);
        }
    }

    void setFunctor(const std::function<Type()> &fun) {
        m_fun = fun;
    }

    std::function<Type()> m_fun;
};

/**
 * @brief Primary expression template base.
 *
 * Specialized by expression type and content.
 */
template <typename Expr, typename Type, IsTrigMode TM>
class Expression : public CalcExprBase<Type, TM> {
};

/**
 * @brief Expression specialization for reactive variables.
 *
 * Allows manual setting and change detection.
 */
template <typename Type, IsTrigMode TM>
class Expression<VarExpr, Type, TM> : public Resource<Type> {
public:
    using Resource<Type>::Resource;

    template <typename T>
    void setValue(T &&t) {
        bool changed = true;
        if constexpr (ComparableType<Type>) {
            changed = this->getValue() != t;
        }
        this->updateValue(std::forward<T>(t));
        this->notify(changed);
    }
};

/**
 * @brief Expression specialization for binary expressions.
 */
template <typename Op, typename L, typename R, IsTrigMode TM>
class Expression<CalcExpr, BinaryOpExpr<Op, L, R>, TM>
    : public CalcExprBase<std::common_type_t<typename L::ValueType, typename R::ValueType>, TM> {
public:
    template <typename T>
        requires(!std::is_same_v<std::decay_t<T>, Expression>)
    Expression(T &&expr) : m_expr(std::forward<T>(expr)) {
    }

protected:
    /// @brief Sets the operator expression as the current source.
    void setOpExpr() {
        this->setSource([this]() {
            return m_expr();
        });
    }

private:
    BinaryOpExpr<Op, L, R> m_expr;
};

} // namespace reaction
