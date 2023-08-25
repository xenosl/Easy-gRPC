#pragma once

#include "ShuHai/gRPC/RpcType.h"

#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/async_stream.h>

namespace ShuHai::gRPC
{
    template<typename T>
    struct StreamingInterfaceTraits;

    template<typename R>
    struct StreamingInterfaceTraits<grpc::ClientReader<R>>
    {
        using ReadType = R;

        static constexpr RpcType RPC_TYPE = RpcType::ServerStream;
    };

    template<typename W>
    struct StreamingInterfaceTraits<grpc::ClientWriter<W>>
    {
        using WriteType = W;

        static constexpr RpcType RPC_TYPE = RpcType::ClientStream;
    };

    template<typename R>
    struct StreamingInterfaceTraits<grpc::ServerReader<R>>
    {
        using ReadType = R;

        static constexpr RpcType RPC_TYPE = RpcType::ClientStream;
    };

    template<typename W>
    struct StreamingInterfaceTraits<grpc::ServerWriter<W>>
    {
        using WriteType = W;

        static constexpr RpcType RPC_TYPE = RpcType::ServerStream;
    };

    template<typename R>
    struct StreamingInterfaceTraits<grpc::ClientAsyncReader<R>>
    {
        using ReadType = R;

        static constexpr RpcType RPC_TYPE = RpcType::ServerStream;
    };

    template<typename W>
    struct StreamingInterfaceTraits<grpc::ClientAsyncWriter<W>>
    {
        using WriteType = W;

        static constexpr RpcType RPC_TYPE = RpcType::ClientStream;
    };

    template<typename W, typename R>
    struct StreamingInterfaceTraits<grpc::ServerAsyncReader<W, R>>
    {
        using WriteType = W;
        using ReadType = R;

        static constexpr RpcType RPC_TYPE = RpcType::ClientStream;
    };

    template<typename W>
    struct StreamingInterfaceTraits<grpc::ServerAsyncWriter<W>>
    {
        using WriteType = W;

        static constexpr RpcType RPC_TYPE = RpcType::ServerStream;
    };

    template<typename W>
    struct StreamingInterfaceTraits<grpc::ServerAsyncResponseWriter<W>>
    {
        using WriteType = W;

        static constexpr RpcType RPC_TYPE = RpcType::UnaryCall;
    };

    template<typename R>
    struct StreamingInterfaceTraits<grpc::ClientAsyncResponseReader<R>>
    {
        using ReadType = R;

        static constexpr RpcType RPC_TYPE = RpcType::UnaryCall;
    };

    template<typename W, typename R>
    struct StreamingInterfaceTraits<grpc::ClientAsyncReaderWriter<W, R>>
    {
        using WriteType = W;
        using ReadType = R;

        static constexpr RpcType RPC_TYPE = RpcType::BidiStream;
    };

    template<typename W, typename R>
    struct StreamingInterfaceTraits<grpc::ServerAsyncReaderWriter<W, R>>
    {
        using WriteType = W;
        using ReadType = R;
        
        static constexpr RpcType RPC_TYPE = RpcType::BidiStream;
    };
}
