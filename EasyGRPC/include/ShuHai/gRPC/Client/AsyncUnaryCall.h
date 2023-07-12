#pragma once

#include "ShuHai/gRPC/Client/AsyncCall.h"
#include "ShuHai/gRPC/Client/AsyncCallError.h"
#include "ShuHai/gRPC/Client/AsyncCallAction.h"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <future>
#include <functional>
#include <cassert>

namespace ShuHai::gRPC::Client
{
    enum class AsyncUnaryResponseMode
    {
        Future,
        Callback
    };

    template<typename CallFunc>
    class AsyncUnaryCall
        : public AsyncCall
        , public std::enable_shared_from_this<AsyncUnaryCall<CallFunc>>
    {
    public:
        SHUHAI_GRPC_CLIENT_EXPAND_AsyncCallTraits(CallFunc);

        using ResponseCallback = std::function<void(std::future<Response>&&)>;
        using DeadCallback = std::function<void(std::shared_ptr<AsyncUnaryCall>)>;

        explicit AsyncUnaryCall(Stub* stub, CallFunc func, const Request& request, grpc::CompletionQueue* cq,
            std::unique_ptr<grpc::ClientContext> context, ResponseCallback responseCallback, DeadCallback deadCallback)
            : AsyncCall(std::move(context))
            , _responseCallback(std::move(responseCallback))
            , _deadCallback(std::move(deadCallback))
        {
            _stream = (stub->*func)(this->_context.get(), request, cq);
            performNewAction<CallFinishAction>(this, &_response, &this->_status);
        }

        std::future<Response> response()
        {
            if (responseMode() != AsyncUnaryResponseMode::Future)
                throw std::logic_error("The function is only available in future mode.");
            return _responsePromise.get_future();
        }

        [[nodiscard]] AsyncUnaryResponseMode responseMode() const
        {
            return _responseCallback ? AsyncUnaryResponseMode::Callback : AsyncUnaryResponseMode::Future;
        }

    private:
        class CallAction : public IAsyncAction
        {
        public:
            explicit CallAction(AsyncUnaryCall* call)
                : _call(call)
            { }

        protected:
            AsyncUnaryCall* const _call;
        };

        class CallFinishAction : public CallAction
        {
        public:
            CallFinishAction(AsyncUnaryCall* call, Response* response, grpc::Status* status)
                : CallAction(call)
                , _response(response)
                , _status(status)
            { }

            void perform() override { this->_call->_stream->Finish(_response, _status, this); }

            void finalizeResult(bool ok) override { this->_call->finalizeFinished(ok); }

        private:
            Response* const _response;
            grpc::Status* const _status;
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

            if (_responseCallback)
                _responseCallback(_responsePromise.get_future());

            _deadCallback(this->shared_from_this());
        }

        std::unique_ptr<StreamingInterface> _stream;
        Response _response;
        std::promise<Response> _responsePromise;
        ResponseCallback _responseCallback;

        DeadCallback _deadCallback;
    };
}
