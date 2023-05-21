#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/CompletionQueueTag.h"
#include "ShuHai/gRPC/AsyncStreamState.h"

#include <grpcpp/server_context.h>

#include <future>
#include <atomic>

namespace ShuHai::gRPC::Server
{
    template<typename TRequestFunc>
    class AsyncClientStreamCallHandler;

    template<typename TRequestFunc>
    class AsyncRequestStreamReader
    {
    public:
        using RequestFunc = TRequestFunc;
        using Request = typename AsyncRequestTraits<TRequestFunc>::RequestType;
        using Response = typename AsyncRequestTraits<TRequestFunc>::ResponseType;
        using StreamReader = typename AsyncRequestTraits<TRequestFunc>::StreamingInterfaceType;

        std::future<bool> moveNext()
        {
            auto state = _state.load(std::memory_order_acquire);
            if (state == AsyncStreamState::Streaming)
                throw std::logic_error("Attempt to move next while the iterator is moving next.");
            if (state == AsyncStreamState::Finished)
                throw std::logic_error("Attempt to move next after the iterator is finished.");

            read();
            return _currentReadyPromise.get_future();
        }

        [[nodiscard]] const Request& current() const { return _current; }

    private:
        friend class AsyncClientStreamCallHandler<RequestFunc>;

        explicit AsyncRequestStreamReader(grpc::ServerContext* context, std::function<void(bool)> onFinished)
            : _reader(context)
            , _onFinished(std::move(onFinished))
        { }

        void prepare()
        {
            _currentReadyPromise = {};
            _state.store(AsyncStreamState::Ready, std::memory_order_release);
        }

        void read()
        {
            _state = AsyncStreamState::Streaming;
            _reader.Read(&_current, new GenericCompletionQueueTag([this](bool ok) { onRead(ok); }));
        }

        void finish(const Response& response)
        {
            _state = AsyncStreamState::Finished;
            _reader.Finish(
                response, grpc::Status::OK, new GenericCompletionQueueTag([this](bool ok) { onFinished(ok); }));
        }

        void onRead(bool ok)
        {
            // ok indicates whether there is a valid message that got read.
            // If not, you know that there are certainly no more messages that can ever be read from this stream, this
            // could happen because the client has done a WritesDone already.

            _currentReadyPromise.set_value(ok);
            _currentReadyPromise = {};

            if (ok)
                _state.store(AsyncStreamState::Ready, std::memory_order_release);
        }

        void onFinished(bool ok)
        {
            // ok means that the data/metadata/status/etc is going to go to the wire.
            // If it is false, it not going to the wire because the call is already dead (i.e., canceled, deadline
            // expired, other side dropped the channel, etc).
            _onFinished(ok);
        }

        std::promise<bool> _currentReadyPromise;
        Request _current;
        std::atomic<AsyncStreamState> _state { AsyncStreamState::Ready };

        StreamReader _reader;

        std::function<void(bool)> _onFinished;
    };

    template<typename TRequestFunc>
    class AsyncClientStreamCallHandler : public AsyncCallHandler<TRequestFunc>
    {
    public:
        using RequestFunc = TRequestFunc;
        using Base = AsyncCallHandler<TRequestFunc>;
        using Service = typename Base::Service;
        using Request = typename Base::Request;
        using Response = typename Base::Response;
        using StreamReader = typename Base::StreamingInterface;
        using ClientStreamReader = AsyncRequestStreamReader<TRequestFunc>;
        using ProcessFunc = std::function<Response(grpc::ServerContext&, ClientStreamReader&)>;

        static_assert(std::is_same_v<grpc::ServerAsyncReader<Response, Request>, StreamReader>);

        AsyncClientStreamCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc)
            : Base(completionQueue, service, requestFunc)
            , _processFunc(std::move(processFunc))
        {
            newHandler();
        }

        ~AsyncClientStreamCallHandler() override { deleteHandlers(); }

    private:
        ProcessFunc _processFunc;

    private:
        class Handler;

        // The handler instance for single call.
        class Handler
        {
        public:
            explicit Handler(AsyncClientStreamCallHandler* owner)
                : _owner(owner)
                , _requestStreamReader(&_context, [this](bool ok) { onFinished(ok); })
            {
                auto service = _owner->_service;
                auto requestFunc = _owner->_requestFunc;
                auto cq = _owner->_completionQueue;

                (service->*requestFunc)(&_context, &_requestStreamReader._reader, cq, cq,
                    new GenericCompletionQueueTag([&](bool ok) { onRequestResult(ok); }));
            }

        private:
            void onRequestResult(bool ok)
            {
                // ok indicates that the RPC has indeed been started.
                // If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

                if (ok)
                {
                    _owner->newHandler();

                    _requestStreamReader.prepare();
                    auto response = _owner->_processFunc(_context, _requestStreamReader);
                    _requestStreamReader.finish(response);
                }
                else
                {
                    _owner->deleteHandler(this);
                }
            }

            void onFinished(bool) { _owner->deleteHandler(this); }

            AsyncClientStreamCallHandler* const _owner;

            grpc::ServerContext _context;
            ClientStreamReader _requestStreamReader;
        };

        void newHandler()
        {
            std::lock_guard l(_handlersMutex);
            _handlers.emplace(new Handler(this));
        }

        void deleteHandler(Handler* handler)
        {
            std::lock_guard l(_handlersMutex);
            auto it = _handlers.find(handler);
            delete *it;
            _handlers.erase(it);
        }

        void deleteHandlers()
        {
            std::lock_guard l(_handlersMutex);

            auto it = _handlers.begin();
            while (it != _handlers.end())
            {
                delete *it;
                it = _handlers.erase(it);
            }
        }

        std::mutex _handlersMutex;
        std::unordered_set<Handler*> _handlers;
    };
}
