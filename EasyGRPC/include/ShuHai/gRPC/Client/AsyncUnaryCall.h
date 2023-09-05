#pragma once

#include "ShuHai/gRPC/Client/AsyncCall.h"
#include "ShuHai/gRPC/Client/AsyncCallError.h"
#include "ShuHai/gRPC/Client/TypeTraits.h"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <asio/dispatch.hpp>
#include <asio/execution_context.hpp>

#include <future>
#include <functional>
#include <cassert>

namespace ShuHai::gRPC::Client
{
    template<typename CallFunc>
    class AsyncUnaryCall
        : public AsyncCall
        , public std::enable_shared_from_this<AsyncUnaryCall<CallFunc>>
    {
    public:
        SHUHAI_GRPC_CLIENT_EXPAND_AsyncCallTraits(CallFunc);

        using ResponseCallback = std::function<void(std::shared_future<Response>)>;
        using DeadCallback = std::function<void(std::shared_ptr<AsyncUnaryCall>)>;

        explicit AsyncUnaryCall(Stub* stub, CallFunc func, const Request& request, grpc::CompletionQueue* cq,
            std::unique_ptr<grpc::ClientContext> context, ResponseCallback responseCallback,
            asio::execution_context* responseCallbackExecutionContext, DeadCallback deadCallback)
            : AsyncCall(std::move(context))
            , _responseCallback(std::move(responseCallback))
            , _responseCallbackExecutionContext(responseCallbackExecutionContext)
            , _deadCallback(std::move(deadCallback))
        {
            _responseFuture = _responsePromise.get_future();
            _stream = (stub->*func)(this->_context.get(), request, cq);
            new CallFinishAction(this, &_response, &this->_status);
        }

        std::shared_future<Response> response() { return _responseFuture; }

    private:
        class CallAction : public IAsyncAction
        {
        public:
            explicit CallAction(AsyncUnaryCall* owner)
                : _owner(owner)
            { }

        protected:
            AsyncUnaryCall* const _owner;
        };

        class CallFinishAction : public CallAction
        {
        public:
            explicit CallFinishAction(AsyncUnaryCall* owner, Response* response, grpc::Status* status)
                : CallAction(owner)
            {
                this->_owner->_stream->Finish(response, status, this);
            }

            void finalizeResult(bool ok) override { this->_owner->finalizeFinished(ok); }
        };

        void finalizeFinished(bool ok)
        {
            // ok should always be true
            assert(ok);

            try
            {
                if (!this->_status.ok())
                    throw AsyncCallError(std::move(this->_status));
                _responsePromise.set_value(std::move(_response));
            }
            catch (...)
            {
                _responsePromise.set_exception(std::current_exception());
            }

            responseCallback();

            _deadCallback(this->shared_from_this());
        }

        void responseCallback()
        {
            if (!_responseCallback)
                return;

            if (_responseCallbackExecutionContext)
            {
                auto ex = asio::get_associated_executor(*_responseCallbackExecutionContext);
                asio::dispatch(ex, [cb = std::move(_responseCallback), f = _responseFuture]() { cb(f); });
            }
            else
            {
                asio::dispatch([cb = std::move(_responseCallback), f = _responseFuture]() { cb(f); });
            }
        }

        std::unique_ptr<StreamingInterface> _stream;
        Response _response;
        std::promise<Response> _responsePromise;
        std::shared_future<Response> _responseFuture;

        ResponseCallback _responseCallback;
        asio::execution_context* _responseCallbackExecutionContext {};

        DeadCallback _deadCallback;
    };
}
