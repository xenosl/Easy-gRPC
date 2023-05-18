#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"

#include <grpcpp/completion_queue.h>
#include <google/protobuf/message.h>

namespace ShuHai::gRPC::Server
{
    class AsyncCallHandlerBase
    {
    public:
        virtual ~AsyncCallHandlerBase() = default;
    };

    template<typename TRequestFunc>
    class AsyncCallHandler : public AsyncCallHandlerBase
    {
    public:
        using RequestFunc = TRequestFunc;
        using Service = typename AsyncRequestTraits<TRequestFunc>::ServiceType;
        using Request = typename AsyncRequestTraits<TRequestFunc>::RequestType;
        using Response = typename AsyncRequestTraits<TRequestFunc>::ResponseType;
        using StreamingInterface = typename AsyncRequestTraits<TRequestFunc>::StreamingInterfaceType;

        static_assert(std::is_base_of_v<grpc::Service, Service>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Request>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Response>);

    protected:
        AsyncCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service, RequestFunc requestFunc)
            : _completionQueue(completionQueue)
            , _service(service)
            , _requestFunc(requestFunc)
        { }

        grpc::ServerCompletionQueue* const _completionQueue;
        Service* const _service;
        const RequestFunc _requestFunc;
    };
}
