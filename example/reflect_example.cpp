/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include <iostream>
#include <string>
#include <tuple>

template <typename T>
struct Field
{
};

template <typename T>
struct my_wrapper
{
    inline static T value;
};

template <typename T, auto... field>
struct private_visitor
{
    friend constexpr auto get_private_ptrs(const my_wrapper<T> &)
    {
        constexpr auto tp = std::make_tuple(field...);
        return tp;
    }
};

#define REFL_PRIVATE(STRUCT, ...)                                       \
    inline constexpr auto get_private_ptrs(const my_wrapper<STRUCT> &); \
    template struct private_visitor<STRUCT, ##__VA_ARGS__>;

struct Dog
{
    bool m_male;
};

struct Person
{
    Field<std::string> m_name;
    Field<int> m_age;
    bool m_male;
};

class PersonPrivate
{
    Field<std::string> m_privateName;
    Field<int> m_privateAge;
    bool m_privateMale;
};
REFL_PRIVATE(PersonPrivate, &PersonPrivate::m_privateName, &PersonPrivate::m_privateAge, &PersonPrivate::m_privateMale)

template <typename T>
inline constexpr T &get_global_value()
{
    return my_wrapper<T>::value;
}

template <auto val>
inline constexpr std::string_view getFunName()
{
#if defined(__GNUC__)
    constexpr std::string_view func_name = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr std::string_view func_name = __FUNCSIG__;
#endif
    size_t pos1 = func_name.find("val = ");
    if (pos1 == std::string_view::npos)
    {
        return {};
    }
    size_t pos2 = func_name.find(';', pos1 + 6);
    if (pos2 == std::string_view::npos)
    {
        return {};
    }
    return func_name.substr(pos1 + 6, pos2 - pos1 - 6);
}

struct AnyType
{
    template <typename T>
    operator T();
};

template <typename T>
consteval size_t countMember(auto &&...Args)
{
    if constexpr (!requires { T{Args...}; })
    {
        return sizeof...(Args) - 1;
    }
    else
    {
        return countMember<T>(Args..., AnyType{});
    }
}

template <typename T>
constexpr size_t member_count_v = countMember<T>();

template <typename T>
struct Is_Field : std::false_type
{
};

template <typename T>
struct Is_Field<Field<T>> : std::true_type
{
};

// PersonPrivate::Field<std::string>* -> Field<std::string>

template <auto MemberPtr>
struct MemberPointerTraits
{
};

template <typename T, typename C, T C::*MemberPtr>
struct MemberPointerTraits<MemberPtr>
{
    using type = T;
    using class_type = C;
};

template <auto MemberPtr>
using member_value_v = typename MemberPointerTraits<MemberPtr>::type;

template <typename T>
concept IsAggregate = std::is_aggregate_v<T>;

template <typename Tuple>
constexpr bool check_field(const Tuple &tup)
{
    bool found = false;
    std::apply([&](const auto &...args)
               { ((found = found || Is_Field<std::decay_t<decltype(args)>>{}), ...); }, tup);

    return found;
}

template <typename T, std::size_t n>
struct ReflectHelper
{
};

#define REF_STRUCT(n, ...)                                \
    template <typename T>                                 \
    struct ReflectHelper<T, n>                            \
    {                                                     \
        static constexpr auto getTuple()                  \
        {                                                 \
            auto &&[__VA_ARGS__] = get_global_value<T>(); \
            return std::tie(__VA_ARGS__);                 \
        }                                                 \
        static constexpr auto reflectFieldImpl()          \
        {                                                 \
            constexpr auto ref_tup = getTuple();          \
            return check_field(ref_tup);                  \
        }                                                 \
    };

REF_STRUCT(1, m0)
REF_STRUCT(2, m0, m1)
REF_STRUCT(3, m0, m1, m2)
REF_STRUCT(4, m0, m1, m2, m3)
REF_STRUCT(5, m0, m1, m2, m3, m4)

template <typename T>
struct ReflectField
{
    static constexpr auto reflect()
    {
        constexpr auto tp = get_private_ptrs(my_wrapper<T>{});
        bool found = false;

        [&]<size_t... Is>(std::index_sequence<Is...>)
        {
            ((found = found || Is_Field<member_value_v<std::get<Is>(tp)>>()), ...);
        }(std::make_index_sequence<std::tuple_size_v<decltype(tp)>>{});

        return found;
    }
};

template <IsAggregate T>
struct ReflectField<T>
{
    static constexpr auto reflect()
    {
        return ReflectHelper<T, member_count_v<T>>::reflectFieldImpl();
    }
};

template <typename T>
constexpr bool reflectField_v = ReflectField<T>::reflect();

int main()
{
    static_assert(!reflectField_v<Dog>);
    static_assert(reflectField_v<Person>);
    static_assert(reflectField_v<PersonPrivate>);
}