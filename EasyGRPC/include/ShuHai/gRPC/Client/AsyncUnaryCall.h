#pragma once

#include "ShuHai/gRPC/Client/AsyncCall.h"
#include "ShuHai/gRPC/Client/AsyncCallError.h"
#include "ShuHai/gRPC/Client/AsyncCallAction.h"
#include "ShuHai/gRPC/Client/StreamActions.h"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <atomic>
#include <future>
#include <functional>
#include <cassert>

namespace ShuHai::gRPC::Client
{
    enum class AsyncUnaryResponseMode
    {
        Block,
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
            unaryCallFinish<Response>(*_stream, &_response, &this->_status, [this](bool ok) { finalizeFinished(ok); });
        }

        std::future<Response> getResponseFuture()
        {
            if (responseMode() != AsyncUnaryResponseMode::Block)
                throw std::logic_error("The function is only available in block mode.");
            return _responsePromise.get_future();
        }

        /**
         * \brief Indicates whether the rpc is returned from server, used for check validity of this->context() and
         *  this->status().
         * \remark When you get context or status from current call, you may want to make sure whether the current call
         *  is returned from server (so that you can get initial and trailing metadata coming from the server or get the
         *  right status of current rpc), the method achieved the goal.
         */
        [[nodiscard]] bool returned() const { return _returned.load(std::memory_order_acquire); }

        [[nodiscard]] AsyncUnaryResponseMode responseMode() const
        {
            return _responseCallback ? AsyncUnaryResponseMode::Callback : AsyncUnaryResponseMode::Block;
        }

    private:
        void finalizeFinished(bool ok)
        {
            // ok should always be true
            assert(ok);

            _returned.store(true, std::memory_order_release);

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

        std::atomic<bool> _returned { false };
    };
}
