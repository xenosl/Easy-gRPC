#pragma once

#include <tuple>
#include <type_traits>

namespace ShuHai
{
    template<typename T>
    using RemoveConstReferenceT = std::remove_const_t<std::remove_reference_t<T>>;

    template<size_t N, typename F>
    struct ArgumentType;

    template<size_t N, typename Ret, typename Class, typename... Args>
    struct ArgumentType<N, Ret (Class::*)(Args...) const>
    {
        using Type = std::tuple_element_t<N, std::tuple<Args...>>;
    };

    template<size_t N, typename Ret, typename Class, typename... Args>
    struct ArgumentType<N, Ret (Class::*)(Args...)>
    {
        using Type = std::tuple_element_t<N, std::tuple<Args...>>;
    };

    template<size_t N, typename F>
    using ArgumentTypeT = typename ArgumentType<N, F>::Type;


    template<typename F>
    struct ResultType;

    template<typename Ret, typename Class, typename... Args>
    struct ResultType<Ret (Class::*)(Args...) const>
    {
        using Type = Ret;
    };

    template<typename Ret, typename Class, typename... Args>
    struct ResultType<Ret (Class::*)(Args...)>
    {
        using Type = Ret;
    };

    template<typename F>
    using ResultTypeT = typename ResultType<F>::Type;


    template<typename F>
    struct ClassType;

    template<typename Ret, typename Class, typename... Args>
    struct ClassType<Ret (Class::*)(Args...) const>
    {
        using Type = Class;
    };

    template<typename Ret, typename Class, typename... Args>
    struct ClassType<Ret (Class::*)(Args...)>
    {
        using Type = Class;
    };

    template<typename F>
    using ClassTypeT = typename ClassType<F>::Type;
}
