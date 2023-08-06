#pragma once

#include <future>
#include <functional>
#include <type_traits>

namespace ShuHai::gRPC
{
    class IAsyncAction
    {
    public:
        virtual ~IAsyncAction() = default;

        virtual void finalizeResult(bool ok) = 0;
    };
}
