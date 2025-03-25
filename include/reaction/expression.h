#ifndef REACTION_EXPRESSION_H
#define REACTION_EXPRESSION_H

#include "reaction/resource.h"
#include "reaction/triggerMode.h"

namespace reaction {

// Forward declaration of DataSource to be used in ExpressionTraits
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
class DataSource;

// General template for ExpressionTraits
template <typename T>
struct ExpressionTraits {
    using Type = T;
};

// Specialization for DataSource without Functor (Base case)
template <typename TriggerPolicy, typename InvalidStrategy, UninvocaCC T>
struct ExpressionTraits<DataSource<TriggerPolicy, InvalidStrategy, T>> {
    using Type = T;
};

// Specialization for DataSource with Functor (Recursive case)
template <typename TriggerPolicy, typename InvalidStrategy, typename Fun, typename... Args>
struct ExpressionTraits<DataSource<TriggerPolicy, InvalidStrategy, Fun, Args...>> {
    using Type = decltype(std::declval<Fun>()(std::declval<typename ExpressionTraits<Args>::Type>()...));
};

// Helper alias to retrieve return type of functors
template <typename Fun, typename... Args>
using ReturnType = typename ExpressionTraits<DataSource<void, void, Fun, Args...>>::Type;

// General Expression class template (for functor-based expressions)
template <typename TriggerPolicy, typename Fun, typename... Args>
class Expression : public Resource<ComplexExpr, ReturnType<Fun, Args...>>, public TriggerPolicy {
public:
    using ValueType = ReturnType<Fun, Args...>;

    // Sets the source data for the expression and updates the functor
    template <typename F, typename... A>
    bool setSource(F &&f, A &&...args) {
        bool repeat = false;

        // Check if we should repeat the update
        if (!this->updateObservers(repeat, [this](bool changed) { this->valueChanged(changed); }, std::forward<A>(args)...)) {
            return false;
        }

        // Choose the right functor based on whether we repeat the update or not
        if (repeat) {
            setFunctor(createUpdateFunRef(std::forward<F>(f), std::forward<A>(args)...));
        } else {
            setFunctor(createGetFunRef(std::forward<F>(f), std::forward<A>(args)...));
        }

        // Handle trigger policy for threshold
        if constexpr (std::is_same_v<TriggerPolicy, ThresholdTrigger>) {
            TriggerPolicy::setRepeatDependent(repeat);
        }

        // Update the value or evaluate the functor
        if constexpr (!std::is_void_v<ValueType>) {
            this->updateValue(evaluate());
        } else {
            evaluate();
        }

        return true;
    }

protected:
    // Evaluates the functor and returns its result
    auto evaluate() const -> std::conditional_t<std::is_void_v<ValueType>, void, ValueType> {
        if constexpr (std::is_void_v<ValueType>) {
            std::invoke(m_fun);
        } else {
            return std::invoke(m_fun);
        }
    }

    // Value change notification based on trigger policy
    void valueChanged(bool changed) {
        if (TriggerPolicy::checkTrigger(changed)) {
            if constexpr (!std::is_void_v<ValueType>) {
                auto oldVal = this->getValue();
                auto newVal = evaluate();
                this->updateValue(newVal);
                this->notifyObservers(oldVal != newVal);
            } else {
                evaluate();
            }
        }
    }

    // Set the functor for the expression
    void setFunctor(std::function<ValueType()> fun) {
        m_fun = std::move(fun);
    }

private:
    std::function<ValueType()> m_fun; // Functor for evaluation
};

// Specialized Expression class template (for simple value-based expressions)
template <typename TriggerPolicy, typename Type>
class Expression<TriggerPolicy, Type> : public Resource<SimpleExpr, std::decay_t<Type>> {
public:
    using Resource<SimpleExpr, std::decay_t<Type>>::Resource;
    using ValueType = Type;

protected:
    // Direct value retrieval for simple expressions
    Type evaluate() const {
        return this->getValue();
    }

    // Set the value of the expression
    template <typename T>
    bool setSource(T &&t) {
        this->updateValue(std::forward<T>(t));
        return true;
    }
};

} // namespace reaction

#endif // REACTION_EXPRESSION_H
