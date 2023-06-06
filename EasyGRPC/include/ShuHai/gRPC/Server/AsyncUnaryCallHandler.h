#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/Server/ServiceRequestAction.h"
#include "ShuHai/gRPC/Server/StreamActions.h"

#include <atomic>

namespace ShuHai::gRPC::Server
{
    template<typename RequestFunc>
    class AsyncUnaryCallHandler : public AsyncCallHandler<RequestFunc>
    {
    public:
        SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(RequestFunc);

        using HandleFunc = std::function<Response(grpc::ServerContext&, const Request&)>;

        explicit AsyncUnaryCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, HandleFunc handleFunc)
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

            ~Call() { }

            grpc::ServerContext context;
            StreamingInterface stream;
            Request request;
        };

        void newCallRequest()
        {
            auto call = new Call();
            auto cq = this->_completionQueue;
            serviceRequest(this->_service, this->_requestFunc, &call->context, &call->request, &call->stream, cq, cq,
                [this, call](bool ok) { finalizeCallRequest(call, ok); });
        }

        void finalizeCallRequest(Call* call, bool ok)
        {
            // ok indicates that the RPC has indeed been started.
            // If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

            if (ok)
            {
                newCallRequest();

                auto response = _handleFunc(call->context, call->request);
                unaryCallFinish<Response>(
                    call->stream, response, grpc::Status::OK, [this, call](bool ok) { finalizeCallFinish(call, ok); });
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
