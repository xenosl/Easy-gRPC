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
    struct AsyncCallTraits
    {
        using StubType = typename FunctionTraits<F>::ClassType;
        using ResponseReaderType = typename FunctionTraits<F>::ResultType::element_type;
        using RequestType = RemoveConstReferenceT<typename FunctionTraits<F>::template ArgumentT<1>>;
        using ResponseType = typename StreamingInterfaceTraits<ResponseReaderType>::ReadType;
        
        static constexpr RpcType RpcType = StreamingInterfaceTraits<ResponseReaderType>::RpcType;
    };
}
