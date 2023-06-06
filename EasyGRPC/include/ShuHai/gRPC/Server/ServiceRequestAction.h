#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/AsyncAction.h"

namespace ShuHai::gRPC::Server
{
    template<typename RequestFunc>
    class ServiceRequestAction;

    template<typename Service, typename Request, typename Response>
    class ServiceRequestAction<AsyncUnaryCallRequestFunc<Service, Request, Response>> : public AsyncAction
    {
    public:
        using RequestFunc = AsyncUnaryCallRequestFunc<Service, Request, Response>;

        ServiceRequestAction(Service* service, RequestFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _service(service)
            , _func(func)
        { }

        void perform(grpc::ServerContext* context, Request* request, grpc::ServerAsyncResponseWriter<Response>* stream,
            grpc::CompletionQueue* newCallCq, grpc::ServerCompletionQueue* notificationCq)
        {
            (_service->*_func)(context, request, stream, newCallCq, notificationCq, this);
        }

    private:
        Service* _service;
        RequestFunc _func;
    };

    template<typename Service, typename Request, typename Response>
    void serviceRequest(Service* service, AsyncUnaryCallRequestFunc<Service, Request, Response> func,
        grpc::ServerContext* context, Request* request, grpc::ServerAsyncResponseWriter<Response>* stream,
        grpc::CompletionQueue* newCallCq, grpc::ServerCompletionQueue* notificationCq,
        std::function<void(bool)> finalizer = nullptr)
    {
        (new ServiceRequestAction<AsyncUnaryCallRequestFunc<Service, Request, Response>>(
             service, func, std::move(finalizer)))
            ->perform(context, request, stream, newCallCq, notificationCq);
    }
}
