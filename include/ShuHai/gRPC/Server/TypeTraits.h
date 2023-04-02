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
    struct AsyncRequestFuncTraits
    {
        using ServiceType = ClassTypeT<F>;
        using ResponseWriterType = std::remove_pointer_t<ArgumentTypeT<2, F>>;
        using RequestType = std::remove_pointer_t<ArgumentTypeT<1, F>>;
        using ResponseType = typename StreamingInterfaceTraits<ResponseWriterType>::MessageType;
    };
}
