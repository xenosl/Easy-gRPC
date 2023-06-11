#pragma once

#include "ShuHai/gRPC/TypeTraits.h"
#include "ShuHai/TypeTraits.h"

#include <google/protobuf/message.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/client_context.h>

#define SHUHAI_GRPC_CLIENT_EXPAND_AsyncCallTraits(F) \
    using Stub = typename AsyncCallTraits<F>::StubType; \
    using Request = typename AsyncCallTraits<F>::RequestType; \
    using Response = typename AsyncCallTraits<F>::ResponseType; \
    using StreamingInterface = typename AsyncCallTraits<F>::StreamingInterfaceType; \
\
    static_assert(std::is_member_function_pointer_v<F>); \
    static_assert(std::is_base_of_v<google::protobuf::Message, Request>); \
    static_assert(std::is_base_of_v<google::protobuf::Message, Response>)

namespace ShuHai::gRPC::Client
{
    /**
     * \brief Deduce types from function Stub::Prepare<RpcName> or Stub::Async<RpcName>.
     * \tparam F Type of function Stub::PrepareAsync<RpcName> or Stub::Async<Rpc> in generated code.
     */
    template<typename F>
    struct AsyncCallTraits;

    template<typename Stub, typename Request, typename Response>
    using AsyncUnaryCallFunc = std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> (Stub::*)(
        grpc::ClientContext*, const Request&, grpc::CompletionQueue*);

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<AsyncUnaryCallFunc<Stub, Request, Response>>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncResponseReader<Response>;

        static constexpr RpcType RpcType = RpcType::UnaryCall;
    };

    template<typename Stub, typename Request, typename Response>
    using AsyncClientStreamFunc = std::unique_ptr<grpc::ClientAsyncWriter<Request>> (Stub::*)(
        grpc::ClientContext*, Response*, grpc::CompletionQueue*, void*);

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<AsyncClientStreamFunc<Stub, Request, Response>>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncWriter<Request>;

        static constexpr RpcType RpcType = RpcType::ClientStream;
    };

    template<typename Stub, typename Request, typename Response>
    using AsyncServerStreamFunc = std::unique_ptr<grpc::ClientAsyncReader<Response>> (Stub::*)(
        grpc::ClientContext*, const Request&, grpc::CompletionQueue*, void*);

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<AsyncServerStreamFunc<Stub, Request, Response>>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncReader<Response>;

        static constexpr RpcType RpcType = RpcType::ServerStream;
    };

    template<typename Stub, typename Request, typename Response>
    using AsyncBidiStreamFunc = std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>> (Stub::*)(
        grpc::ClientContext*, grpc::CompletionQueue*, void*);

    template<typename Stub, typename Request, typename Response>
    struct AsyncCallTraits<AsyncBidiStreamFunc<Stub, Request, Response>>
    {
        using StubType = Stub;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ClientAsyncReaderWriter<Request, Response>;

        static constexpr RpcType RpcType = RpcType::BidiStream;
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
    constexpr RpcType rpcTypeOf()
    {
        return AsyncCallTraits<F>::RpcType;
    }

    template<typename F, RpcType T, typename EnabledType>
    using EnableIfRpcTypeMatch = std::enable_if_t<rpcTypeOf<F>() == T, EnabledType>;

    template<typename EnabledType, typename F, RpcType... RpcTypes>
    using EnableIfAnyRpcTypeMatch = std::enable_if_t<((rpcTypeOf<F>() == RpcTypes) || ...), EnabledType>;
}
