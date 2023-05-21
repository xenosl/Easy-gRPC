#pragma once

#include "ShuHai/gRPC/TypeTraits.h"
#include "ShuHai/TypeTraits.h"

#include <grpcpp/completion_queue.h>
#include <grpcpp/client_context.h>

namespace ShuHai::gRPC::Client
{
    /**
     * \brief Deduce types from function Stub::Prepare<RpcName> or Stub::Async<RpcName>.
     * \tparam F Type of function Stub::PrepareAsync<RpcName> or Stub::Async<Rpc> in generated code.
     */
    template<typename F>
    struct AsyncCallTraits;

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> (Stub::*)(
        grpc::ClientContext*, const Request&, grpc::CompletionQueue*)>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncResponseReader<Response>;

        static constexpr RpcType RpcType = RpcType::NORMAL_RPC;
    };

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<std::unique_ptr<grpc::ClientAsyncReader<Response>> (Stub::*)(
        grpc::ClientContext*, const Request&, grpc::CompletionQueue*, void*)>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncReader<Response>;

        static constexpr RpcType RpcType = RpcType::SERVER_STREAMING;
    };

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<std::unique_ptr<grpc::ClientAsyncWriter<Request>> (Stub::*)(
        grpc::ClientContext*, Response*, grpc::CompletionQueue*, void*)>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncWriter<Request>;

        static constexpr RpcType RpcType = RpcType::CLIENT_STREAMING;
    };

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>> (Stub::*)(
        grpc::ClientContext*, grpc::CompletionQueue*, void*)>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncReaderWriter<Request, Response>;

        static constexpr RpcType RpcType = RpcType::BIDI_STREAMING;
    };

    template<typename F>
    using StubTypeOf = typename AsyncCallTraits<F>::StubType;

    template<typename F>
    using RequestTypeOf = typename AsyncCallTraits<F>::RequestType;

    template<typename F>
    using ResponseTypeOf = typename AsyncCallTraits<F>::ResponseType;

    template<typename F>
    using StreamingInterfaceTypeOf = typename AsyncCallTraits<F>::StreamingInterfaceType;

    template<typename F>
    static constexpr RpcType rpcTypeOf()
    {
        return AsyncCallTraits<F>::RpcType;
    }

    template<typename F, RpcType T, typename Result>
    using EnableIfRpcTypeMatch = std::enable_if_t<AsyncCallTraits<F>::RpcType == T, Result>;
}
