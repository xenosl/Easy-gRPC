#pragma once

#include "ShuHai/gRPC/Client/AsyncClientStreamWriter.h"
#include "ShuHai/gRPC/Client/AsyncCall.h"
#include "ShuHai/gRPC/Client/TypeTraits.h"

#include <future>
#include <atomic>
#include <memory>

namespace ShuHai::gRPC::Client
{
    template<typename CallFunc>
    class AsyncClientStreamWriter;

    template<typename CallFunc>
    class AsyncClientStreamCall
        : public AsyncCall
        , public std::enable_shared_from_this<AsyncClientStreamCall<CallFunc>>
    {
    public:
        SHUHAI_GRPC_CLIENT_EXPAND_AsyncCallTraits(CallFunc);
        using RequestStream = AsyncClientStreamWriter<CallFunc>;

        AsyncClientStreamCall(
            Stub* stub, CallFunc func, std::unique_ptr<grpc::ClientContext> context, grpc::CompletionQueue* cq)
            : AsyncCall(std::move(context))
            , _requestStreamFuture(_requestStreamPromise.get_future())
            , _responseFuture(_responsePromise.get_future())
        {
            _stream = (new CallAction(this))->perform(stub, func, this->_context.get(), &_response, cq);
            _requestStream = new RequestStream(cq, *_stream, this->_status, [this]() { onRequestStreamFinish(); });
        }

        ~AsyncClientStreamCall() override
        {
            delete _requestStream;
            _requestStream = nullptr;
        }

        [[nodiscard]] std::shared_future<RequestStream*> requestStream() { return _requestStreamFuture; }

        [[nodiscard]] std::shared_future<Response> response() const { return _responseFuture; }

    private:
        class CallAction : public IAsyncAction
        {
        public:
            explicit CallAction(AsyncClientStreamCall* owner)
                : _owner(owner)
            { }

            std::unique_ptr<StreamingInterface> perform(Stub* stub, AsyncClientStreamFunc<Stub, Request, Response> func,
                grpc::ClientContext* context, Response* response, grpc::CompletionQueue* cq)
            {
                return (stub->*func)(context, response, cq, this);
            }

            void finalizeResult(bool ok) override { _owner->markRequestStreamReady(); }

        private:
            AsyncClientStreamCall* const _owner;
        };

        void markRequestStreamReady() { _requestStreamPromise.set_value(_requestStream); }

        void onRequestStreamFinish() { _responsePromise.set_value(_response); }

        std::unique_ptr<StreamingInterface> _stream;

        RequestStream* _requestStream;
        std::promise<RequestStream*> _requestStreamPromise;
        std::shared_future<RequestStream*> _requestStreamFuture;

        Response _response;
        std::promise<Response> _responsePromise;
        std::shared_future<Response> _responseFuture;
    };
}
