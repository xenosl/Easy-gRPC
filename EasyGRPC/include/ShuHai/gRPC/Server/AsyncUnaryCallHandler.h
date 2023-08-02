#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandler.h"
#include "ShuHai/gRPC/IAsyncAction.h"

namespace ShuHai::gRPC::Server
{
    template<typename RequestFunc>
    class AsyncUnaryCallHandler : public AsyncCallHandler<RequestFunc>
    {
    public:
        SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(RequestFunc);

        using HandleFunc = std::function<Response(grpc::ServerContext&, const Request&)>;

        AsyncUnaryCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service, RequestFunc requestFunc,
            HandleFunc handleFunc)
            : AsyncCallHandler<RequestFunc>(completionQueue, service, requestFunc)
            , _handleFunc(std::move(handleFunc))
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
            { }

            void perform()
            {
                auto handler = this->_handler;
                auto call = this->_call;
                auto service = handler->_service;
                auto func = handler->_requestFunc;
                auto cq = handler->_completionQueue;
                (service->*func)(&call->context, &call->request, &call->stream, cq, cq, this);
            }

            void finalizeResult(bool ok) override { this->_handler->finalizeCallRequest(this->_call, ok); }
        };

        class CallFinishAction : public CallHandlerAction
        {
        public:
            explicit CallFinishAction(AsyncUnaryCallHandler* handler, Call* call)
                : CallHandlerAction(handler, call)
            { }

            void perform(const Response& response, const grpc::Status& status)
            {
                this->_call->stream.Finish(response, status, this);
            }

            void finalizeResult(bool ok) override { this->_handler->finalizeCallFinish(this->_call, ok); }
        };

        void newCallRequest()
        {
            auto call = new Call();
            (new ServiceRequestAction(this, call))->perform();
        }

        void finalizeCallRequest(Call* call, bool ok)
        {
            // ok indicates that the RPC has indeed been started.
            // If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

            if (ok)
            {
                newCallRequest();

                auto response = _handleFunc(call->context, call->request);
                (new CallFinishAction(this, call))->perform(response, grpc::Status::OK);
            }
            else
            {
                delete call;
            }
        }

        void finalizeCallFinish(Call* call, bool ok)
        {
            // ok means that the data/metadata/status/etc is going to go to the wire.
            // If it is false, it not going to the wire because the call is already dead (i.e., canceled, deadline
            // expired, other side dropped the channel, etc).

            delete call;
        }

        HandleFunc _handleFunc;
    };
}
