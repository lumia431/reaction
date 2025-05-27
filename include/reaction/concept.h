#include <concepts>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace reaction {
// ==================== Forward declarations ====================
struct VarExpr;
struct VoidWrapper;
class FieldBase;

template <typename T, typename... Args>
class ReactImpl;

template <typename T>
class React;

template <typename Op, typename L, typename R>
class BinaryOpExpr;

template <typename T>
struct ValueWrapper;

// ==================== Basic type concepts ====================

template <typename T, typename U>
concept Convertable = std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>;

template <typename T>
concept IsVarExpr = std::is_same_v<T, VarExpr>;

template <typename T>
concept HasField = requires(T t) {
    { t.getId() } -> std::same_as<uint64_t>;
    requires std::is_base_of_v<FieldBase, std::decay_t<T>>;
};

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

// ==================== Type Traits ====================

template <typename T>
struct IsReact : std::false_type {
    using Type = T;
};

template <typename T>
struct IsReact<React<T>> : std::true_type {
    using Type = T;
};

template <typename T>
struct ExpressionTraits {
    using type = T;
};

template <NonInvocableType T>
struct ExpressionTraits<React<ReactImpl<T>>> {
    using type = T;
};

template <typename Fun, typename... Args>
struct ExpressionTraits<React<ReactImpl<Fun, Args...>>> {
    using RawType = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::type...>;
    using type = std::conditional_t<VoidType<RawType>, VoidWrapper, RawType>;
};

template <typename Fun, typename... Args>
using ReturnType = typename ExpressionTraits<React<ReactImpl<Fun, Args...>>>::type;

template <typename T>
struct BinaryOpExprTraits : std::false_type {};

template <typename Op, typename L, typename R>
struct BinaryOpExprTraits<BinaryOpExpr<Op, L, R>> : std::true_type {};

template <typename T>
concept IsBinaryOpExpr = BinaryOpExprTraits<T>::value;

template <typename T>
using ExprWrapper = std::conditional_t<
    IsReact<T>::value || IsBinaryOpExpr<T>,
    T,
    ValueWrapper<T>>;

template <typename L, typename R>
concept HasCustomOp = IsReact<std::decay_t<L>>::value ||
    IsReact<std::decay_t<R>>::value ||
    IsBinaryOpExpr<std::decay_t<L>> ||
    IsBinaryOpExpr<std::decay_t<R>>;

} // namespace reaction