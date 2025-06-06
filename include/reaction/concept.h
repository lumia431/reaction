/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <concepts>
#include <memory>

namespace reaction {

// ==================== Forward declarations ====================
class FieldBase;
class ObserverNode;
struct VarExpr;
struct VoidWrapper;
struct ReactAsKey;

template <typename TM, typename IS, typename Type, typename... Args>
class ReactImpl;

template <typename T>
class React;

template <typename Op, typename L, typename R>
class BinaryOpExpr;

template <typename T>
struct ValueWrapper;

// ==================== Basic type concepts ====================

struct AnyTp {
    template <typename T>
    operator T() { return T{}; }
};

template <typename T, typename U>
concept Convertable = std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>;

template <typename T>
concept IsVarExpr = std::is_same_v<T, VarExpr>;

template <typename T>
concept IsReactAsKey = std::is_same_v<T, ReactAsKey>;

template <typename T>
concept ConstType = std::is_const_v<std::remove_reference_t<T>>;

template <typename T>
concept VoidType = std::is_void_v<T> || std::is_same_v<T, VoidWrapper>;

template <typename T>
concept InvocableType = std::is_invocable_v<std::decay_t<T>>;

template <typename T>
concept NonInvocableType = !InvocableType<T>;

template <typename... Args>
concept HasArguments = sizeof...(Args) > 0;

template <typename T>
concept ComparableType = requires(T &a, T &b) {
    { a == b } -> std::convertible_to<bool>;
};

template <typename T>
concept HasField = requires(T t) {
    { t.getId() } -> std::same_as<uint64_t>;
    requires std::is_base_of_v<FieldBase, std::decay_t<T>>;
};

template <typename T>
concept IsTrigMode = requires(T t) {
    { t.checkTrig() } -> std::same_as<bool>;
};

template <typename T>
concept IsInvaStra = requires(T t, AnyTp ds) {
    { t.handleInvalid(ds) } -> std::same_as<void>;
};

template <typename T>
concept IsReactSource = requires(T t) {
    requires requires { { t.shared_from_this() } -> std::same_as<std::shared_ptr<ObserverNode>>; };
};

template <typename T>
concept IsDataSource = requires {
    typename T::ValueType;
    requires IsReactSource<T> && !std::is_void_v<typename T::ValueType>;
};

// ==================== Type Traits =======================

template <typename T>
struct ReactTraits : std::false_type {
    using Type = T;
};

template <typename T>
struct ReactTraits<React<T>> : std::true_type {
    using Type = T;
};

template <typename T>
concept IsReact = ReactTraits<T>::value;

template <typename T>
struct ExpressionTraits {
    using type = T;
};

template <typename TM, typename IS, NonInvocableType T>
struct ExpressionTraits<React<ReactImpl<TM, IS, T>>> {
    using type = T;
};

template <typename TM, typename IS, typename Fun, typename... Args>
struct ExpressionTraits<React<ReactImpl<TM, IS, Fun, Args...>>> {
    using RawType = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::type...>;
    using type = std::conditional_t<VoidType<RawType>, VoidWrapper, RawType>;
};

template <typename Fun, typename... Args>
using ReturnType = typename ExpressionTraits<React<ReactImpl<void, void, Fun, Args...>>>::type;

template <typename T>
struct BinaryOpExprTraits : std::false_type {};

template <typename Op, typename L, typename R>
struct BinaryOpExprTraits<BinaryOpExpr<Op, L, R>> : std::true_type {};

template <typename T>
concept IsBinaryOpExpr = BinaryOpExprTraits<T>::value;

template <typename T>
using ExprTraits = std::conditional_t<
    IsReact<T> || IsBinaryOpExpr<T>,
    T,
    ValueWrapper<T>>;

template <typename L, typename R>
concept HasReactOp = IsReact<std::decay_t<L>> ||
                     IsReact<std::decay_t<R>> ||
                     IsBinaryOpExpr<std::decay_t<L>> ||
                     IsBinaryOpExpr<std::decay_t<R>>;

} // namespace reaction