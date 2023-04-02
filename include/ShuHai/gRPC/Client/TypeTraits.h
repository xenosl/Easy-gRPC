#pragma once

#include "ShuHai/gRPC/TypeTraits.h"
#include "ShuHai/TypeTraits.h"
#include "ShuHai/FunctionTraits.h"


namespace ShuHai::gRPC::Client
{
    /**
     * \brief Deduce types from function Stub::Prepare<RpcName> or Stub::Async<RpcName>.
     * \tparam F Type of function Stub::PrepareAsync<RpcName> or Stub::Async<Rpc> in generated code.
     */
    template<typename F>
    struct AsyncFuncTraits
    {
        using StubType = ClassTypeT<F>;
        using ResponseReaderType = typename ResultTypeT<F>::element_type;
        using RequestType = RemoveConstReferenceT<ArgumentTypeT<1, F>>;
        using ResponseType = typename StreamingInterfaceTraits<ResponseReaderType>::MessageType;
    };
}
