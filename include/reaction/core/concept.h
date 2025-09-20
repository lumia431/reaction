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
struct Void;
struct ChangeTrig;

template <typename Op, typename L, typename R>
class BinaryOpExpr;

template <typename Op, typename T>
class UnaryOpExpr;

template <typename T>
struct ValueWrapper;

// ==================== Basic type concepts ====================

/**
 * @brief Placeholder type used for generic substitution.
 */
struct AnyTp {
    template <typename T>
    operator T();
};

/**
 * @brief Checks if type T is convertible to type U (after decay).
 */
template <typename T, typename U>
concept Convertable = std::is_convertible_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

/**
 * @brief Determines if the type is a variable expression.
 */
template <typename T>
concept IsVarExpr = std::is_same_v<T, VarExpr>;

/**
 * @brief Determines if the type is a change-trigger marker.
 */
template <typename T>
concept IsChangeTrig = std::is_same_v<T, ChangeTrig>;

/**
 * @brief Checks if the type is const (after removing reference).
 */
template <typename T>
concept ConstType = std::is_const_v<std::remove_reference_t<T>>;

/**
 * @brief Determines if the type represents a Void type.
 */
template <typename T>
concept VoidType = std::is_void_v<T> || std::is_same_v<T, Void>;

/**
 * @brief Checks if a type is invocable (i.e., a callable function, lambda, etc).
 */
template <typename T>
concept InvocableType = std::is_invocable_v<std::remove_cvref_t<T>>;

/**
 * @brief Logical negation of InvocableType.
 */
template <typename T>
concept NonInvocableType = !InvocableType<T>;

/**
 * @brief Checks if the argument pack is non-empty.
 */
template <typename... Args>
concept HasArguments = sizeof...(Args) > 0;

/**
 * @brief Checks if the type supports equality comparison.
 */
template <typename T>
concept ComparableType = requires(T &a, T &b) {
    { a == b } -> std::convertible_to<bool>;
};

/**
 * @brief Checks if a type implements the field interface.
 */
template <typename T>
concept HasField = requires(T t) {
    { t.getId() } -> std::same_as<uint64_t>;
    requires std::is_base_of_v<FieldBase, std::remove_cvref_t<T>>;
};

/**
 * @brief Checks if a type satisfies the trigger mode interface.
 */
template <typename T>
concept IsTrigger = requires(T t) {
    { t.checkTrig() } -> std::same_as<bool>;
};

/**
 * @brief Checks if a type satisfies the invalidation strategy interface.
 */
template <typename T>
concept IsInvalidation = requires(T t, AnyTp ds) {
    { t.handleInvalid(ds) } -> std::same_as<void>;
};

/**
 * @brief Checks if a type can serve as a reactive data source.
 *        It must support shared_from_this() and return ObserverNode.
 */
template <typename T>
concept IsReactSource = requires(T t) {
    { t.shared_from_this() } -> std::same_as<std::shared_ptr<ObserverNode>>;
};

/**
 * @brief Internal implementation class for a reactive node.
 *
 * @tparam Expr  Expression type.
 * @tparam Type  Evaluated result type of the reactive node.
 * @tparam IV    Invalidation strategy type.
 * @tparam TR    Triggering mode type.
 */
template <typename Expr, typename Type, IsInvalidation IV, IsTrigger TR>
class ReactImpl;

/**
 * @brief Public wrapper of a reactive node.
 *
 * @tparam Expr  Expression type.
 * @tparam Type  Final result type.
 * @tparam IV    Invalidation strategy.
 * @tparam TR    Triggering mode.
 */
template <typename Expr, typename Type, IsInvalidation IV, IsTrigger TR>
class React;

// ==================== Type Traits =======================

/**
 * @brief Fallback trait to mark non-reactive types.
 */
template <typename T>
struct ReactTraits : std::false_type {
    using type = T;
};

/**
 * @brief Specialization of ReactTraits for React types.
 */
template <typename Expr, typename Type, IsInvalidation IV, IsTrigger TR>
struct ReactTraits<React<Expr, Type, IV, TR>> : std::true_type {
    using type = Type;
};

/**
 * @brief Concept to determine if a type is a React instance.
 */
template <typename T>
concept IsReact = ReactTraits<std::remove_cvref_t<T>>::value;

/**
 * @brief Logical negation of IsReact.
 */
template <typename T>
concept NonReact = !IsReact<T>;

/**
 * @brief Extracts the return type from a callable expression using the React argument types.
 */
template <typename Fun, typename... Args>
struct ExpressionTraits {
    using raw_type = std::invoke_result_t<Fun, typename ReactTraits<Args>::type...>;
    using type = std::conditional_t<VoidType<raw_type>, Void, std::remove_cvref_t<raw_type>>;
};

/**
 * @brief Alias to get the return type of a callable using React traits.
 */
template <typename Fun, typename... Args>
using ReturnType = typename ExpressionTraits<std::remove_cvref_t<Fun>, std::remove_cvref_t<Args>...>::type;

/**
 * @brief Fallback trait for identifying binary operation expressions.
 */
template <typename T>
struct BinaryOpExprTraits : std::false_type {};

/**
 * @brief Specialization for BinaryOpExpr types.
 */
template <typename Op, typename L, typename R>
struct BinaryOpExprTraits<BinaryOpExpr<Op, L, R>> : std::true_type {};

/**
 * @brief Fallback trait for identifying unary operation expressions.
 */
template <typename T>
struct UnaryOpExprTraits : std::false_type {};

/**
 * @brief Specialization for UnaryOpExpr types.
 */
template <typename Op, typename T>
struct UnaryOpExprTraits<UnaryOpExpr<Op, T>> : std::true_type {};

/**
 * @brief Concept to check if a type is a binary operation expression.
 */
template <typename T>
concept IsBinaryOpExpr = BinaryOpExprTraits<T>::value;

/**
 * @brief Concept to check if a type is a unary operation expression.
 */
template <typename T>
concept IsUnaryOpExpr = UnaryOpExprTraits<T>::value;

/**
 * @brief Concept to check if a type is either a binary or unary operation expression.
 */
template <typename T>
concept IsOpExpr = IsBinaryOpExpr<T> || IsUnaryOpExpr<T>;

/**
 * @brief Resolves the proper expression type.
 *        If it's already a React, BinaryOpExpr, or UnaryOpExpr, return as-is.
 *        Otherwise, wrap it using ValueWrapper.
 */
template <typename T>
using ExprTraits = std::conditional_t<IsReact<T> || IsBinaryOpExpr<T> || IsUnaryOpExpr<T>, T, ValueWrapper<T>>;

/**
 * @brief Concept to determine if either operand is reactive or a binary expression.
 */
template <typename L, typename R>
concept HasReactOp = IsReact<std::remove_cvref_t<L>> ||
                     IsReact<std::remove_cvref_t<R>> ||
                     IsBinaryOpExpr<std::remove_cvref_t<L>> ||
                     IsBinaryOpExpr<std::remove_cvref_t<R>>;

} // namespace reaction
