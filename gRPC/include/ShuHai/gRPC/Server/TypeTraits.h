#pragma once

#include "ShuHai/gRPC/TypeTraits.h"
#include "ShuHai/TypeTraits.h"
#include "ShuHai/FunctionTraits.h"

namespace ShuHai::gRPC::Server
{
    /**
     * \brief Deduce types from function AsyncService::Request<RpcName>.
     * \tparam F Type of function AsyncService::Request<RpcName> in generated code.
     */
    template<typename F>
    struct AsyncRequestTraits
    {
        using ServiceType = typename FunctionTraits<F>::ClassType;
        using ResponseWriterType = std::remove_pointer_t<typename FunctionTraits<F>::template ArgumentT<2>>;
        using RequestType = std::remove_pointer_t<typename FunctionTraits<F>::template ArgumentT<1>>;
        using ResponseType = typename StreamingInterfaceTraits<ResponseWriterType>::WriteType;
    };
}
