#ifndef REACTION_CONCEPT_H
#define REACTION_CONCEPT_H
#include <type_traits>
#include <memory>
#include <concepts>

namespace reaction {

struct AnyType {
    // Conversion operator to allow AnyType to be used as any type T
    template <typename T>
    operator T() {
        return T{};
    }
};

template <typename T>
concept TriggerCC = requires(T t, bool trigger) {
    { t.checkTrigger(trigger) } -> std::same_as<bool>;
};

struct DataNode;
struct ActionNode;
struct FieldNode;
template <typename T>
concept NodeCC = requires(T t) {
    requires std::is_same_v<typename T::SourceType, DataNode> || std::is_same_v<typename T::SourceType, ActionNode> || std::is_same_v<typename T::SourceType, FieldNode>;
};

class ObserverDataNode;
class ObserverActionNode;
class ObserverFieldNode;
template <typename T>
concept SourceCC = requires(T t) {
    requires requires { { t.getShared() } -> std::same_as<std::shared_ptr<ObserverDataNode>>; } || requires { { t.getShared() } -> std::same_as<std::shared_ptr<ObserverActionNode>>; } || requires { { t.getShared() } -> std::same_as<std::shared_ptr<ObserverFieldNode>>; };
};

template <typename T>
concept ActionSourceCC = requires {
    typename T::ExprType::ValueType;
    requires SourceCC<T> && std::is_same_v<typename T::ExprType::ValueType, void>;
};

template <typename T>
concept DataSourceCC = requires {
    typename T::ExprType::ValueType;
    requires SourceCC<T> && !std::is_same_v<typename T::ExprType::ValueType, void>;
};

template <typename T>
concept InvalidCC = requires(T t) {
    requires requires(AnyType ds) {
        { t.handleInvalid(ds) } -> std::same_as<void>;
    };
};

template <typename T>
concept ConstCC = requires {
    requires std::is_const_v<T>;
};

template <typename T>
concept InvocaCC = requires {
    requires std::is_invocable_v<T>;
};

template <typename T>
concept UninvocaCC = requires {
    requires !InvocaCC<T>;
};

template <typename... Args>
concept ArgSizeOverZeroCC = requires {
    requires sizeof...(Args) > 0;
};

struct FieldStructBase;
template <typename T>
concept HasFieldCC = requires {
    requires std::is_base_of_v<FieldStructBase, T>;
};

template <typename T>
concept CompareCC = requires(T &a, T &b) {
    { a == b } -> std::convertible_to<bool>;
};

} // namespace reaction

#endif // REACTION_CONCEPT_H