#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/CompletionQueueNotification.h"

#include <ShuHai/FunctionTraits.h>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <unordered_set>
#include <stdexcept>
#include <type_traits>

namespace ShuHai::gRPC::Server
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
        using ResponseWriter = typename Base::ResponseWriter;
        using ProcessFunc = std::function<void(grpc::ServerContext&, const Request&, Response&)>;

        explicit AsyncUnaryCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc)
            : AsyncCallHandler<TRequestFunc>(completionQueue, service, requestFunc)
            , _processFunc(std::move(processFunc))
        {
            newHandler();
        }

        ~AsyncUnaryCallHandler() { deleteAllHandlers(); }

    private:
        ProcessFunc _processFunc;

        // Handlers ----------------------------------------------------------------------------------------------------
    private:
        class Handler;
        using HandlerCallback = void (AsyncUnaryCallHandler::*)(Handler*, bool);

        class Handler
        {
        public:
            explicit Handler(AsyncUnaryCallHandler* owner, HandlerCallback onProcess, HandlerCallback onFinish)
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
            HandlerCallback _onProcess;
            HandlerCallback _onFinish;

            grpc::ServerContext _context;
            ResponseWriter _responseWriter;
            Request _request;
        };

        void newHandler()
        {
            auto handler =
                new Handler(this, &AsyncUnaryCallHandler::onHandlerProcess, &AsyncUnaryCallHandler::onHandlerFinish);
            _handlers.emplace(handler);
            handler->request();
        }

        bool deleteHandler(Handler* handler)
        {
            auto it = _handlers.find(handler);
            if (it == _handlers.end())
                return false;
            deleteHandler(it);
            return true;
        }

        auto deleteHandler(typename std::unordered_set<Handler*>::iterator it)
        {
            assert(it != _handlers.end());
            auto handler = *it;
            it = _handlers.erase(it);
            delete handler;
            return it;
        }

        void deleteAllHandlers()
        {
            auto it = _handlers.begin();
            while (it != _handlers.end())
                it = deleteHandler(it);
        }

        void onHandlerProcess(Handler* handler, bool ok)
        {
            if (ok)
                newHandler();
            else
                deleteHandler(handler);
        }

        void onHandlerFinish(Handler* handler, bool ok) { deleteHandler(handler); }

        std::unordered_set<Handler*> _handlers;
    };
}
