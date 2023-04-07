#pragma once

#include <grpcpp/grpcpp.h>

namespace ShuHai::gRPC
{
    template<typename T>
    struct StreamingInterfaceTraits;

    template<typename M>
    struct StreamingInterfaceTraits<grpc::ClientReader<M>>
    {
        using MessageType = M;
    };

    template<typename M>
    struct StreamingInterfaceTraits<grpc::ClientWriter<M>>
    {
        using MessageType = M;
    };

    template<typename M>
    struct StreamingInterfaceTraits<grpc::ClientAsyncReader<M>>
    {
        using MessageType = M;
    };

    template<typename M>
    struct StreamingInterfaceTraits<grpc::ClientAsyncWriter<M>>
    {
        using MessageType = M;
    };

    template<typename M>
    struct StreamingInterfaceTraits<grpc::ServerAsyncResponseWriter<M>>
    {
        using MessageType = M;
    };

    template<typename M>
    struct StreamingInterfaceTraits<grpc::ClientAsyncResponseReader<M>>
    {
        using MessageType = M;
    };
}
