#pragma once

#include "ShuHai/gRPC/TypeTraits.h"
#include "ShuHai/TypeTraits.h"

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

namespace ShuHai::gRPC::Server
{
    /**
     * \brief Deduce types from function AsyncService::Request<RpcName>.
     * \tparam F Type of function AsyncService::Request<RpcName> in generated code.
     */
    template<typename F>
    struct AsyncRequestTraits;

    template<typename Service, typename Request, typename Response>
    using AsyncUnaryCallRequestFunc = void (Service::*)(grpc::ServerContext*, Request*,
        grpc::ServerAsyncResponseWriter<Response>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*);

    template<typename Service, typename Request, typename Response>
    struct AsyncRequestTraits<AsyncUnaryCallRequestFunc<Service, Request, Response>>
    {
        using ServiceType = Service;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ServerAsyncResponseWriter<Response>;

        static constexpr RpcType RPC_TYPE = RpcType::UnaryCall;
    };

    template<typename Service, typename Request, typename Response>
    using ClientStreamRequestFunc = void (Service::*)(grpc::ServerContext*, grpc::ServerAsyncReader<Response, Request>*,
        grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*);

    template<typename Service, typename Request, typename Response>
    struct AsyncRequestTraits<ClientStreamRequestFunc<Service, Request, Response>>
    {
        using ServiceType = Service;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ServerAsyncReader<Response, Request>;

        static constexpr RpcType RPC_TYPE = RpcType::ClientStream;
    };

    template<typename Service, typename Request, typename Response>
    using ServerStreamRequestFunc = void (Service::*)(grpc::ServerContext*, Request*,
        grpc::ServerAsyncWriter<Response>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*);

    template<typename Service, typename Request, typename Response>
    struct AsyncRequestTraits<ServerStreamRequestFunc<Service, Request, Response>>
    {
        using ServiceType = Service;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ServerAsyncWriter<Response>;

        static constexpr RpcType RPC_TYPE = RpcType::ServerStream;
    };

    template<typename Service, typename Request, typename Response>
    using BidiStreamRequestFunc = void (Service::*)(grpc::ServerContext*,
        grpc::ServerAsyncReaderWriter<Response, Request>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*);

    template<typename Service, typename Request, typename Response>
    struct AsyncRequestTraits<BidiStreamRequestFunc<Service, Request, Response>>
    {
        using ServiceType = Service;
        using RequestType = Request;
        using ResponseType = Response;
        using StreamingInterfaceType = grpc::ServerAsyncReaderWriter<Response, Request>;

        static constexpr RpcType RPC_TYPE = RpcType::BidiStream;
    };

    template<typename F>
    using ServiceTypeOf = typename AsyncRequestTraits<F>::ServiceType;

    template<typename F>
    using RequestTypeOf = typename AsyncRequestTraits<F>::RequestType;

    template<typename F>
    using ResponseTypeOf = typename AsyncRequestTraits<F>::ResponseType;

    template<typename F>
    using StreamingInterfaceTypeOf = typename AsyncRequestTraits<F>::StreamingInterfaceType;

    template<typename F>
    static constexpr RpcType rpcTypeOf()
    {
        return AsyncRequestTraits<F>::RPC_TYPE;
    }

    template<typename F, RpcType T, typename EnabledType>
    using EnableIfRpcTypeMatch = std::enable_if_t<rpcTypeOf<F>() == T, EnabledType>;

    template<typename EnabledType, typename F, RpcType... RpcTypes>
    using EnableIfAnyRpcTypeMatch = std::enable_if_t<((rpcTypeOf<F>() == RpcTypes) || ...), EnabledType>;
}

#define SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(F) \
    using RequestFunc = F; \
    using Service = typename ::ShuHai::gRPC::Server::AsyncRequestTraits<F>::ServiceType; \
    using Request = typename ::ShuHai::gRPC::Server::AsyncRequestTraits<F>::RequestType; \
    using Response = typename ::ShuHai::gRPC::Server::AsyncRequestTraits<F>::ResponseType; \
    using StreamingInterface = typename ::ShuHai::gRPC::Server::AsyncRequestTraits<F>::StreamingInterfaceType; \
    static constexpr gRPC::RpcType RPC_TYPE = ::ShuHai::gRPC::Server::AsyncRequestTraits<F>::RPC_TYPE; \
\
    static_assert(std::is_member_function_pointer_v<F>); \
    static_assert(std::is_base_of_v<grpc::Service, Service>); \
    static_assert(std::is_base_of_v<google::protobuf::Message, Request>); \
    static_assert(std::is_base_of_v<google::protobuf::Message, Response>)
