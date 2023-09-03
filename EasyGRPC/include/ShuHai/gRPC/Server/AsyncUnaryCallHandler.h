#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandler.h"
#include "ShuHai/gRPC/IAsyncAction.h"

#include <grpcpp/alarm.h>

#include <asio/post.hpp>
#include <asio/execution_context.hpp>

namespace ShuHai::gRPC::Server
{
    template<typename RequestFuncType>
    class AsyncUnaryCallHandler : public AsyncCallHandler<RequestFuncType>
    {
    public:
        SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(RequestFuncType);

        using HandleFunc = std::function<Response(grpc::ServerContext&, const Request&)>;

        AsyncUnaryCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service, RequestFunc requestFunc,
            HandleFunc handleFunc, asio::execution_context* handleFuncExecutionContext)
            : AsyncCallHandler<RequestFunc>(completionQueue, service, requestFunc)
            , _handleFunc(std::move(handleFunc))
            , _handleFuncExecutionContext(handleFuncExecutionContext)
        {
            newCallRequest();
        }

    private:
        struct Call
        {
            Call()
                : stream(&context)
            { }

            grpc::ServerContext context;
            StreamingInterface stream;
            Request request;
            Response response;
        };

        class CallHandlerAction : public IAsyncAction
        {
        public:
            CallHandlerAction(AsyncUnaryCallHandler* handler, Call* call)
                : _handler(handler)
                , _call(call)
            { }

        protected:
            AsyncUnaryCallHandler* const _handler;
            Call* const _call;
        };

        class ServiceRequestAction : public CallHandlerAction
        {
        public:
            ServiceRequestAction(AsyncUnaryCallHandler* handler, Call* call)
                : CallHandlerAction(handler, call)
            {
                auto service = handler->_service;
                auto func = handler->_requestFunc;
                auto cq = handler->_completionQueue;
                (service->*func)(&call->context, &call->request, &call->stream, cq, cq, this);
            }

            void finalizeResult(bool ok) override { this->_handler->finalizeCallRequest(this->_call, ok); }
        };

        class CallHandlingAction : public CallHandlerAction
        {
        public:
            CallHandlingAction(AsyncUnaryCallHandler* handler, Call* call, asio::execution_context* executionContext)
                : CallHandlerAction(handler, call)
            {
                const auto& func = this->_handler->_handleFunc;
                if (executionContext)
                {
                    auto ex = asio::get_associated_executor(*executionContext);
                    asio::post(ex,
                        [&func, call, this]()
                        {
                            call->response = func(call->context, call->request);
                            notifyFinalize();
                        });
                }
                else
                {
                    asio::post(
                        [&func, call, this]()
                        {
                            call->response = func(call->context, call->request);
                            notifyFinalize();
                        });
                }
            }

            void finalizeResult(bool ok) override { this->_handler->finalizeCallHandling(this->_call, ok); }

        private:
            void notifyFinalize() { _alarm.Set(this->_handler->_completionQueue, gpr_now(GPR_CLOCK_REALTIME), this); }

            grpc::Alarm _alarm;
        };

        class CallFinishAction : public CallHandlerAction
        {
        public:
            CallFinishAction(AsyncUnaryCallHandler* handler, Call* call, const grpc::Status& status)
                : CallHandlerAction(handler, call)
            {
                this->_call->stream.Finish(call->response, status, this);
            }

            void finalizeResult(bool ok) override { this->_handler->finalizeCallFinish(this->_call, ok); }
        };

        void newCallRequest()
        {
            auto call = new Call();
            new ServiceRequestAction(this, call);
        }

        void finalizeCallRequest(Call* call, bool ok)
        {
            // ok indicates that the RPC has indeed been started.
            // If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

            if (ok)
            {
                newCallRequest();

                new CallHandlingAction(this, call, _handleFuncExecutionContext);
            }
            else
            {
                delete call;
            }
        }

        void finalizeCallHandling(Call* call, bool ok)
        {
            if (ok)
                new CallFinishAction(this, call, grpc::Status::OK);
            else
                delete call;
        }

        void finalizeCallFinish(Call* call, bool ok)
        {
            // ok means that the data/metadata/status/etc is going to go to the wire.
            // If it is false, it not going to the wire because the call is already dead (i.e., canceled, deadline
            // expired, other side dropped the channel, etc).

            delete call;
        }

        HandleFunc _handleFunc;
        asio::execution_context* _handleFuncExecutionContext {};
    };
}
