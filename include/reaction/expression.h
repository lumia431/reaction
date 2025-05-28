#include "reaction/resource.h"
#include <tuple>
namespace reaction {

struct VarExpr {};
struct CalcExpr {};

template <typename Op, typename L, typename R>
class BinaryOpExpr {
public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;
    template <typename Left, typename Right>
    BinaryOpExpr(Left &&l, Right &&r, Op o = Op{}) : left(std::forward<Left>(l)), right(std::forward<Right>(r)), op(o) {
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

template <typename Fun, typename... Args>
class Expression : public Resource<ReturnType<Fun, Args...>> {
public:
    using ValueType = ReturnType<Fun, Args...>;
    using ExprType = CalcExpr;

    template <typename F, typename... A>
    ReactionError setSource(F &&f, A &&...args) {
        if constexpr (std::convertible_to<ReturnType<std::decay_t<F>, std::decay_t<A>...>, ValueType>) {
            if (!this->updateObservers(args.getPtr()...)) {
                return ReactionError::CycleDepErr;
            }

            setFunctor(createFun(std::forward<F>(f), std::forward<A>(args)...));

            valueChanged();
        } else {
            return ReactionError::ReturnTypeErr;
        }
        return ReactionError::NoErr;
    }

    void addObCb(NodePtr node) {
        this->addOneObserver(node);
    }

protected:
    void valueChanged() override {
        evaluate();
        this->notify();
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

    void evaluate() {
        if constexpr (VoidType<ValueType>) {
            std::invoke(m_fun);
        } else {
            this->updateValue(std::invoke(m_fun));
        }
    }

    void setFunctor(const std::function<ValueType()> &fun) {
        m_fun = fun;
    }

    std::function<ValueType()> m_fun;
};

template <NonInvocableType Type>
class Expression<Type> : public Resource<Type> {
public:
    using ValueType = Type;
    using ExprType = VarExpr;
    using Resource<Type>::Resource;
};

template <typename Op, typename L, typename R>
class Expression<BinaryOpExpr<Op, L, R>>
    : public Expression<std::function<typename std::common_type_t<typename L::ValueType, typename R::ValueType>()>> {
public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;
    using ExprType = CalcExpr;
    template <typename T>
    Expression(T &&expr) : m_expr(std::forward<T>(expr)) {
    }

protected:
    ReactionError setOpExpr() {
        return this->setSource([this]() {
            return m_expr();
        });
    }

private:
    BinaryOpExpr<Op, L, R> m_expr;
};
} // namespace reaction