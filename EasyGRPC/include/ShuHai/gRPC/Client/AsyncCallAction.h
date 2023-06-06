#pragma once

#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/AsyncAction.h"

namespace ShuHai::gRPC::Client
{
    template<typename CallFunc>
    class AsyncCallAction;

    /**
     * \brief Async-call action for client stream rpc on client-side.
     */
    template<typename Stub, typename Request, typename Response>
    class AsyncCallAction<AsyncUnaryCallFunc<Stub, Request, Response>> : public AsyncAction
    {
    public:
        using CallFunc = AsyncUnaryCallFunc<Stub, Request, Response>;

        AsyncCallAction(Stub* stub, CallFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stub(stub)
            , _func(func)
        { }

        std::unique_ptr<grpc::ClientAsyncWriter<Request>> AsyncRequestStream(
            grpc::ClientContext* context, Response* response, grpc::CompletionQueue* cq)
        {
            return (_stub->*_func)(context, response, cq, this);
        }

    private:
        Stub* const _stub;
        const CallFunc _func;
    };

    /**
     * \brief Async-call action for server stream rpc on client-side.
     */
    template<typename Stub, typename Request, typename Response>
    class AsyncCallAction<std::unique_ptr<grpc::ClientAsyncReader<Response>> (Stub::*)(
        grpc::ClientContext*, const Request&, grpc::CompletionQueue*, void*)> : public AsyncAction
    {
    public:
        using CallFunc = std::unique_ptr<grpc::ClientAsyncReader<Response>> (Stub::*)(
            grpc::ClientContext*, const Request&, grpc::CompletionQueue*, void*);

        AsyncCallAction(Stub* stub, CallFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stub(stub)
            , _func(func)
        { }

        std::unique_ptr<grpc::ClientAsyncReader<Response>> perform(
            ::grpc::ClientContext* context, const Request& request, ::grpc::CompletionQueue* cq)
        {
            return (_stub->*_func)(context, request, cq, this);
        }

    private:
        Stub* const _stub;
        const CallFunc _func;
    };
}
