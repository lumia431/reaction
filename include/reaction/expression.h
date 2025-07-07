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
 * @brief Marker type for variable expressions.
 */
struct VarExpr {};

/**
 * @brief Marker type for calculated expressions.
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
    using value_type = typename std::remove_cvref_t<std::common_type_t<typename L::value_type, typename R::value_type>>;

    template <typename Left, typename Right>
    BinaryOpExpr(Left &&l, Right &&r, Op o = Op{})
        : m_left(std::forward<Left>(l)), m_right(std::forward<Right>(r)), m_op(o) {}

    /// @brief Evaluates the expression.
    auto operator()() const {
        return calculate();
    }

    /// @brief Implicit conversion to value type (evaluates expression).
    operator value_type() {
        return calculate();
    }

private:
    auto calculate() const {
        return m_op(m_left(), m_right());
    }

    L m_left;
    R m_right;
    [[no_unique_address]] Op m_op;
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
    using value_type = std::remove_cvref_t<T>;
    T m_value;

    template <typename Type>
    ValueWrapper(Type &&t) : m_value(std::forward<Type>(t)) {}

    const T &operator()() const {
        return m_value;
    }
};

/**
 * @brief Creates a binary expression from two operands and an operator.
 */
template <typename Op, typename L, typename R>
auto make_binary_expr(L &&l, R &&r) {
    return BinaryOpExpr<Op, ExprTraits<std::remove_cvref_t<L>>, ExprTraits<std::remove_cvref_t<R>>>(std::forward<L>(l), std::forward<R>(r));
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
            throw std::runtime_error("return type cannot reset another!");
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
                return Void{};
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
            bool change = true;
            if constexpr (!VoidType<Type>) {
                change = this->updateValue(evaluate());
            } else {
                evaluate();
            }
            if (this->batchCount == 0) {
                this->notify(change);
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
        bool changed = this->updateValue(std::forward<T>(t));
        if (this->batchCount == 0) {
            this->notify(changed);
        }
    }
};

/**
 * @brief Expression specialization for binary expressions.
 */
template <typename Op, typename L, typename R, IsTrigMode TM>
class Expression<CalcExpr, BinaryOpExpr<Op, L, R>, TM>
    : public CalcExprBase<std::common_type_t<typename L::value_type, typename R::value_type>, TM> {
public:
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, Expression>)
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
