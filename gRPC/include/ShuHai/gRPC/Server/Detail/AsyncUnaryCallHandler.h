#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/Server/Detail/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/CompletionQueueNotification.h"

#include <ShuHai/FunctionTraits.h>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <unordered_set>
#include <stdexcept>
#include <type_traits>

namespace ShuHai::gRPC::Server::Detail
{
    template<typename TRequestFunc>
    class AsyncUnaryCallHandler : public AsyncCallHandler<TRequestFunc>
    {
    public:
        using RequestFunc = TRequestFunc;
        using Base = AsyncCallHandler<RequestFunc>;
        using Service = typename Base::Service;
        using Request = typename Base::Request;
        using Response = typename Base::Response;
        using ProcessFunc = std::function<void(grpc::ServerContext&, const Request&, Response&)>;

        explicit AsyncUnaryCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc)
            : AsyncCallHandler<TRequestFunc>(completionQueue, service, requestFunc)
            , _processFunc(std::move(processFunc))
        {
            newCall();
        }

        ~AsyncUnaryCallHandler() { deleteAllCalls(); }

    private:
        class Call;
        using CallCallback = void (AsyncUnaryCallHandler::*)(Call*, bool);

        class Call
        {
        public:
            explicit Call(AsyncUnaryCallHandler* owner, CallCallback onProcess, CallCallback onFinish)
                : _owner(owner)
                , _onProcess(onProcess)
                , _onFinish(onFinish)
                , _responseWriter(&_context)
            { }

            void request()
            {
                auto cq = _owner->_completionQueue;
                auto service = _owner->_service;
                auto requestFunc = _owner->_requestFunc;

                (service->*requestFunc)(
                    &_context, &_request, &_responseWriter, cq, cq, new GcqNotification([&](bool ok) { process(ok); }));
            }

        private:
            void process(bool ok)
            {
                (_owner->*_onProcess)(this, ok);

                if (!ok)
                    return;

                Response response;
                _owner->_processFunc(_context, _request, response);
                _responseWriter.Finish(response, grpc::Status::OK, new GcqNotification([&](bool ok) { finish(ok); }));
            }

            void finish(bool ok) { (_owner->*_onFinish)(this, ok); }

            AsyncUnaryCallHandler* const _owner;
            CallCallback _onProcess;
            CallCallback _onFinish;

            grpc::ServerContext _context;
            grpc::ServerAsyncResponseWriter<Response> _responseWriter;
            Request _request;
        };

        ProcessFunc _processFunc;

        void newCall()
        {
            auto call = new Call(this, &AsyncUnaryCallHandler::onCallProcess, &AsyncUnaryCallHandler::onCallFinish);
            _calls.emplace(call);
            call->request();
        }

        bool deleteCall(Call* call)
        {
            auto it = _calls.find(call);
            if (it == _calls.end())
                return false;
            deleteCall(it);
            return true;
        }

        auto deleteCall(typename std::unordered_set<Call*>::iterator it)
        {
            assert(it != _calls.end());
            auto call = *it;
            it = _calls.erase(it);
            delete call;
            return it;
        }

        void deleteAllCalls()
        {
            auto it = _calls.begin();
            while (it != _calls.end())
                it = deleteCall(it);
        }

        void onCallProcess(Call* call, bool ok)
        {
            newCall();

            if (!ok)
                deleteCall(call);
        }

        void onCallFinish(Call* call, bool ok) { deleteCall(call); }

        std::unordered_set<Call*> _calls;
    };
}
