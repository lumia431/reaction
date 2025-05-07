#include <concepts>

namespace reaction
{
    // ==================== Forward declarations ====================
    struct VarExpr;

    template <typename T, typename... Args>
    class ReactImpl;

    template <typename T>
    class React;

    // ==================== Basic type concepts ====================

    template <typename T, typename U>
    concept Convertable = std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>;

    template <typename T>
    concept IsVarExpr = std::is_same_v<T, VarExpr>;

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

    template <typename T>
    struct IsReact{
        using Type = T;
    };

    template <typename T>
    struct IsReact<React<T>>{
        using Type = T;
    };

    template <typename T>
    using ReactType = typename IsReact<T>::Type;
}