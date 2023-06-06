#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

namespace ShuHai
{
    template<typename F>
    struct FunctionTraits;

    template<typename Result, typename... Args>
    struct FunctionTraits<Result (*)(Args...)>
    {
        using ResultType = Result;

        template<size_t I>
        struct Argument
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgumentT = typename Argument<I>::Type;
    };

    template<typename Result, typename Class, typename... Args>
    struct FunctionTraits<Result (Class::*)(Args...) const>
    {
        static constexpr size_t ArgumentCount = sizeof...(Args);

        using ClassType = Class;
        using ResultType = Result;

        template<size_t I>
        struct Argument
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgumentT = typename Argument<I>::Type;
    };

    template<typename Result, typename Class, typename... Args>
    struct FunctionTraits<Result (Class::*)(Args...)>
    {
        static constexpr size_t ArgumentCount = sizeof...(Args);

        using ClassType = Class;
        using ResultType = Result;

        template<size_t I>
        struct Argument
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgumentT = typename Argument<I>::Type;
    };

    template<typename Result, typename... Args>
    struct FunctionTraits<std::function<Result(Args...)>>
    {
        static constexpr size_t ArgumentCount = sizeof...(Args);

        using ResultType = Result;

        template<size_t I>
        struct Argument
        {
            using Type = std::tuple_element_t<I, std::tuple<Args...>>;
        };

        template<size_t I>
        using ArgumentT = typename Argument<I>::Type;
    };
}
