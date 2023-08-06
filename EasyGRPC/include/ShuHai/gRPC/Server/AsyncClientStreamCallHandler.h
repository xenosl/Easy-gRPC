#pragma once

#include "ShuHai/gRPC/Server/AsyncClientStreamReader.h"
#include "ShuHai/gRPC/Server/AsyncCallHandler.h"

#include <unordered_set>

namespace ShuHai::gRPC::Server
{
    template<typename RequestFuncType>
    class AsyncClientStreamCallHandler : public AsyncCallHandler<RequestFuncType>
    {
    public:
        SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(RequestFuncType);

        using StreamReader = AsyncClientStreamReader<RequestFunc>;
        using HandleFunc = std::function<Response(grpc::ServerContext&, StreamReader&)>;

        AsyncClientStreamCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, HandleFunc handleFunc)
            : AsyncCallHandler<RequestFunc>(completionQueue, service, requestFunc)
            , _service(service)
            , _handleFunc(handleFunc)
        {
            new RequestStreamAction(this);
        }

    private:
        class Action : public IAsyncAction
        {
        public:
            explicit Action(AsyncClientStreamCallHandler* owner)
                : _owner(owner)
            { }

        protected:
            AsyncClientStreamCallHandler* const _owner;
        };

        class RequestStreamAction : public Action
        {
        public:
            explicit RequestStreamAction(AsyncClientStreamCallHandler* owner)
                : Action(owner)
            {
                auto service = owner->_service;
                auto func = owner->_requestFunc;
                auto cq = owner->_completionQueue;

                _streamReader =
                    new StreamReader(service, cq, [this](auto r) { this->_owner->onStreamReaderFinished(r); });
                auto& context = _streamReader->_context;
                auto& stream = _streamReader->_stream;

                (service->*func)(&context, &stream, cq, cq, this);
            }

            void finalizeResult(bool ok) override
            {
                // ok indicates that the RPC has indeed been started.
                // If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

                if (ok)
                    this->_owner->onNewStreamReaderReady(_streamReader);
                else
                    delete _streamReader;
            }

        private:
            StreamReader* _streamReader {};
        };

        class HandleStreamAction : public Action
        {
        public:
            explicit HandleStreamAction(AsyncClientStreamCallHandler* owner)
                : Action(owner)
            { }

            void finalizeResult(bool ok) override { }
        };

        class FinishAction : public Action
        {
        public:
            explicit FinishAction(AsyncClientStreamCallHandler* owner)
                : Action(owner)
            { }

            void finalizeResult(bool ok) override { }
        };

        void onNewStreamReaderReady(StreamReader* streamReader)
        {
            new RequestStreamAction(this);

            _streamReaders.emplace(streamReader);
            //auto response = _handleFunc(streamReader->_context, *streamReader);
            assert("Not implemented");
        }

        void onStreamReaderFinished(StreamReader* streamReader)
        {
            _streamReaders.erase(streamReader);
            delete streamReader;
        }

        Service* const _service;
        HandleFunc _handleFunc;
        std::unordered_set<StreamReader*> _streamReaders;
    };
}
