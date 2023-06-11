#pragma once

#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/AsyncAction.h"

namespace ShuHai::gRPC::Client
{
    template<typename CallFunc>
    class AsyncStubAction : public AsyncAction
    {
    public:
        using Stub = StubTypeOf<CallFunc>;

        AsyncStubAction(Stub* stub, CallFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stub(stub)
            , _func(func)
        { }

    protected:
        Stub* const _stub;
        const CallFunc _func;
    };

    template<typename CallFunc>
    class AsyncCallAction;

    /**
     * \brief Async-call action for client stream rpc on client-side.
     */
    template<typename Stub, typename Request, typename Response>
    class AsyncCallAction<AsyncClientStreamFunc<Stub, Request, Response>>
        : public AsyncStubAction<AsyncClientStreamFunc<Stub, Request, Response>>
    {
    public:
        using CallFunc = AsyncClientStreamFunc<Stub, Request, Response>;

        AsyncCallAction(Stub* stub, CallFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncStubAction<AsyncClientStreamFunc<Stub, Request, Response>>(stub, func, std::move(finalizer))
        { }

        std::unique_ptr<grpc::ClientAsyncWriter<Request>> perform(
            grpc::ClientContext* context, Response* response, grpc::CompletionQueue* cq)
        {
            return (this->_stub->*this->_func)(context, response, cq, this);
        }
    };

    template<typename Stub, typename Request, typename Response>
    std::unique_ptr<grpc::ClientAsyncWriter<Request>> asyncCall(Stub* stub,
        AsyncClientStreamFunc<Stub, Request, Response> func, grpc::ClientContext* context, Response* response,
        grpc::CompletionQueue* cq, std::function<void(bool)> finalizer = nullptr)
    {
        return (new AsyncCallAction<AsyncClientStreamFunc<Stub, Request, Response>>(stub, func, std::move(finalizer)))
            ->perform(context, response, cq);
    }

    /**
     * \brief Async-call action for server stream rpc on client-side.
     */
    template<typename Stub, typename Request, typename Response>
    class AsyncCallAction<AsyncServerStreamFunc<Stub, Request, Response>>
        : public AsyncStubAction<AsyncServerStreamFunc<Stub, Request, Response>>
    {
    public:
        using CallFunc = AsyncServerStreamFunc<Stub, Request, Response>;

        AsyncCallAction(Stub* stub, CallFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncStubAction<AsyncServerStreamFunc<Stub, Request, Response>>(stub, func, std::move(finalizer))
        { }

        std::unique_ptr<grpc::ClientAsyncReader<Response>> perform(
            ::grpc::ClientContext* context, const Request& request, ::grpc::CompletionQueue* cq)
        {
            return (this->_stub->*this->_func)(context, request, cq, this);
        }
    };

    /**
     * \brief Async-call action for bidirectional stream rpc on client-side.
     */
    template<typename Stub, typename Request, typename Response>
    class AsyncCallAction<AsyncBidiStreamFunc<Stub, Request, Response>>
        : public AsyncStubAction<AsyncBidiStreamFunc<Stub, Request, Response>>
    {
    public:
        using CallFunc = AsyncBidiStreamFunc<Stub, Request, Response>;

        AsyncCallAction(Stub* stub, CallFunc func, std::function<void(bool)> finalizer = nullptr)
            : AsyncStubAction<AsyncBidiStreamFunc<Stub, Request, Response>>(stub, func, std::move(finalizer))
        { }

        std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>> perform(
            grpc::ClientContext* context, grpc::CompletionQueue* cq)
        {
            return (this->_stub->*this->_func)(context, cq, this);
        }
    };
}
