#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace ShuHai
{
    template<typename F>
    struct FunctionTraits;

    template<typename Ret, typename... Args>
    struct FunctionTraits<Ret (*)(Args...)>
    {
        using ResultType = Ret;

        template<size_t I>
        struct Arg
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgT = typename Arg<I>::Type;
    };

    template<typename Ret, typename Class, typename... Args>
    struct FunctionTraits<Ret (Class::*)(Args...) const>
    {
        static constexpr size_t ArgumentCount = sizeof...(Args);

        using ClassType = Class;
        using ResultType = Ret;

        template<size_t I>
        struct Arg
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgT = typename Arg<I>::Type;
    };

    template<typename Ret, typename Class, typename... Args>
    struct FunctionTraits<Ret (Class::*)(Args...)>
    {
        static constexpr size_t ArgumentCount = sizeof...(Args);

        using ClassType = Class;
        using ResultType = Ret;

        template<size_t I>
        struct Arg
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgT = typename Arg<I>::Type;
    };

    template<typename Ret, typename... Args>
    struct FunctionTraits<std::function<Ret(Args...)>>
    {
        static constexpr size_t ArgumentCount = sizeof...(Args);

        using ResultType = Ret;

        template<size_t I>
        struct Arg
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgT = typename Arg<I>::Type;
    };
}
