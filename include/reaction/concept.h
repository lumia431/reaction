#include <concepts>
#include <memory>

namespace reaction
{
    // ==================== Forward declarations ====================
    struct VarExpr;
    class FieldBase;

    template <typename T, typename... Args>
    class ReactImpl;

    template <typename T>
    class React;

    class ObserverNode;
    class FieldBase;

    using NodePtr = std::shared_ptr<ObserverNode>;

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
    concept VoidType = std::is_void_v<T>;

    template <typename T>
    concept IsReactNode = requires(T t) {
        { t.shared_from_this() } -> std::same_as<NodePtr>;
    };

    template <typename T>
    concept IsDataReact = requires(T t) {
        typename T::ValueType;
        requires IsReactNode<T> && !VoidType<typename T::ValueType>;
    };

    // ==================== Type Traits ====================

    template <typename T>
    struct ExpressionTraits
    {
        using Type = T;
    };

    template <typename T>
    struct ExpressionTraits<React<ReactImpl<T>>>
    {
        using Type = T;
    };

    template <typename Fun, typename... Args>
    struct ExpressionTraits<React<ReactImpl<Fun, Args...>>>
    {
        using Type = std::invoke_result_t<Fun, typename ExpressionTraits<Args>::Type...>;
    };

    template <typename Fun, typename... Args>
    using ReturnType = typename ExpressionTraits<React<ReactImpl<Fun, Args...>>>::Type;
}