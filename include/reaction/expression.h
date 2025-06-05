#pragma once

#include "reaction/resource.h"
#include "reaction/triggerMode.h"

namespace reaction {

struct VarExpr {};
struct CalcExpr {};

template <typename Op, typename L, typename R>
class BinaryOpExpr {
public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;

    template <typename Left, typename Right>
    BinaryOpExpr(Left &&l, Right &&r, Op o = Op{}) : left(std::forward<Left>(l)),
                                                     right(std::forward<Right>(r)), op(o) {
    }

    auto operator()() const {
        return calculate();
    }

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

template <typename T>
struct ValueWrapper {
    using ValueType = T;
    T value;

    template <typename Type>
    ValueWrapper(Type &&t) : value(std::forward<Type>(t)) {
    }
    const T &operator()() const {
        return value;
    }
};

template <typename Op, typename L, typename R>
auto make_binary_expr(L &&l, R &&r) {
    return BinaryOpExpr<Op, ExprWrapper<std::decay_t<L>>, ExprWrapper<std::decay_t<R>>>(
        std::forward<L>(l),
        std::forward<R>(r));
}

template <typename L, typename R>
    requires HasCustomOp<L, R>
auto operator+(L &&l, R &&r) {
    return make_binary_expr<AddOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasCustomOp<L, R>
auto operator*(L &&l, R &&r) {
    return make_binary_expr<MulOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasCustomOp<L, R>
auto operator-(L &&l, R &&r) {
    return make_binary_expr<SubOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires HasCustomOp<L, R>
auto operator/(L &&l, R &&r) {
    return make_binary_expr<DivOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename TM, typename Fun, typename... Args>
class Expression : public Resource<ReturnType<Fun, Args...>>, public TM {
public:
    using ValueType = ReturnType<Fun, Args...>;
    using ExprType = CalcExpr;

    template <typename F, typename... A>
    void setSource(F &&f, A &&...args) {
        if constexpr (std::convertible_to<ReturnType<std::decay_t<F>, std::decay_t<A>...>, ValueType>) {
            this->updateObservers(args.getPtr()...);
            setFunctor(createFun(std::forward<F>(f), std::forward<A>(args)...));

            if constexpr (!VoidType<ValueType>) {
                this->updateValue(evaluate());
            } else {
                evaluate();
            }
        }
    }

    void addObCb(NodePtr node) {
        this->addOneObserver(node);
    }

private:
    template <typename F, typename... A>
    auto createFun(F &&f, A &&...args) {
        return [f = std::forward<F>(f), ... args = args.getPtr()]() {
            if constexpr (VoidType<ValueType>) {
                std::invoke(f, args->get()...);
                return VoidWrapper{};
            } else {
                return std::invoke(f, args->get()...);
            }
        };
    }

    void valueChanged(bool changed = true) override {
        if constexpr (std::is_same_v<TM, ChangeTrig>) {
            this->setChanged(changed);
        }

        if (TM::checkTrigger()) {
            if constexpr (!VoidType<ValueType>) {
                auto oldVal = this->getValue();
                auto newVal = evaluate();
                this->updateValue(newVal);
                if constexpr (ComparableType<ValueType>) {
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

    auto evaluate() const {
        if constexpr (VoidType<ValueType>) {
            std::invoke(m_fun);
        } else {
            return std::invoke(m_fun);
        }
    }

    void setFunctor(const std::function<ValueType()> &fun) {
        m_fun = fun;
    }

    std::function<ValueType()> m_fun;
};

template <typename TM, NonInvocableType Type>
class Expression<TM, Type> : public Resource<Type> {
public:
    using ValueType = Type;
    using ExprType = VarExpr;
    using Resource<Type>::Resource;
};

template <typename TM, typename Op, typename L, typename R>
class Expression<TM, BinaryOpExpr<Op, L, R>>
    : public Expression<TM, std::function<std::common_type_t<typename L::ValueType, typename R::ValueType>()>> {
public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;
    using ExprType = CalcExpr;
    template <typename T>
        requires(!std::is_same_v<std::decay_t<T>, Expression>)
    Expression(T &&expr) : m_expr(std::forward<T>(expr)) {
    }

protected:
    void setOpExpr() {
        this->setSource([this]() {
            return m_expr();
        });
    }

private:
    BinaryOpExpr<Op, L, R> m_expr;
};
} // namespace reaction