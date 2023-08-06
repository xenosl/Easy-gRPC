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
        using StreamWriter = AsyncClientStreamWriter<CallFunc>;

        AsyncClientStreamCall(
            Stub* stub, CallFunc func, std::unique_ptr<grpc::ClientContext> context, grpc::CompletionQueue* cq)
            : AsyncCall(std::move(context))
            , _streamWriterFuture(_streamWriterPromise.get_future())
            , _responseFuture(_responsePromise.get_future())
        {
            _stream = (new CallAction(this))->perform(stub, func, this->_context.get(), &_response, cq);
            _streamWriter = new StreamWriter(cq, *_stream, this->_status, [this]() { onStreamWriterFinish(); });
        }

        ~AsyncClientStreamCall() override
        {
            delete _streamWriter;
            _streamWriter = nullptr;
        }

        [[nodiscard]] std::shared_future<StreamWriter*> streamWriter() { return _streamWriterFuture; }

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

            void finalizeResult(bool ok) override { _owner->markStreamWriterReady(); }

        private:
            AsyncClientStreamCall* const _owner;
        };

        void markStreamWriterReady() { _streamWriterPromise.set_value(_streamWriter); }

        void onStreamWriterFinish() { _responsePromise.set_value(_response); }

        std::unique_ptr<StreamingInterface> _stream;

        StreamWriter* _streamWriter;
        std::promise<StreamWriter*> _streamWriterPromise;
        std::shared_future<StreamWriter*> _streamWriterFuture;

        Response _response;
        std::promise<Response> _responsePromise;
        std::shared_future<Response> _responseFuture;
    };
}
