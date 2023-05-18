#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/CompletionQueueTag.h"
#include "ShuHai/gRPC/AsyncReaderState.h"

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
            ensureMoveNext();
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
            _state.store(AsyncReaderState::ReadyRead, std::memory_order_release);
        }

        void read()
        {
            _state = AsyncReaderState::Reading;
            _reader.Read(&_current, new GenericCompletionQueueTag([this](bool ok) { onRead(ok); }));
        }

        void finish(const Response& response)
        {
            _state = AsyncReaderState::Finished;
            _reader.Finish(
                response, grpc::Status::OK, new GenericCompletionQueueTag([this](bool ok) { onFinished(ok); }));
        }

        void ensureMoveNext()
        {
            if (_state == AsyncReaderState::Reading)
                throw std::logic_error("Attempt to move next while the iterator is moving next.");
            if (_state == AsyncReaderState::Finished)
                throw std::logic_error("Attempt to move next after the iterator is finished.");
        }

        void onRead(bool ok)
        {
            // ok indicates whether there is a valid message that got read.
            // If not, you know that there are certainly no more messages that can ever be read from this stream, this
            // could happen because the client has done a WritesDone already.

            _currentReadyPromise.set_value(ok);
            _currentReadyPromise = {};

            if (ok)
                _state.store(AsyncReaderState::ReadyRead, std::memory_order_release);
            else
                finish();
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
        std::atomic<AsyncReaderState> _state { AsyncReaderState::ReadyRead };

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
        using ProcessFunc = std::function<void(grpc::ServerContext&, StreamReader&)>;

        static_assert(std::is_same_v<grpc::ServerAsyncReader<Response, Request>, StreamReader>);

        AsyncClientStreamCallHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc)
            : _completionQueue(completionQueue)
            , _service(service)
            , _requestFunc(requestFunc)
            , _processFunc(std::move(processFunc))
        {
            new Handler(this);
        }

        ~AsyncClientStreamCallHandler() override { destroyHandlers(); }

    private:
        grpc::ServerCompletionQueue* const _completionQueue;
        Service* const _service;
        const RequestFunc _requestFunc;
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

                (service->*requestFunc)(&_context, &_requestStreamReader, cq, cq,
                    new GenericCompletionQueueTag([&](bool ok) { onRequestResult(ok); }));

                _owner->_handlers.emplace(this);
            }

            ~Handler() { _owner->_handlers.erase(this); }

        private:
            void onRequestResult(bool ok)
            {
                // ok indicates that the RPC has indeed been started.
                // If it is false, the server has been Shutdown before this particular call got matched to an incoming RPC.

                if (ok)
                {
                    new Handler(_owner);

                    _requestStreamReader.prepare();
                    _owner->_processFunc(_context, _requestStreamReader);
                }
                else
                {
                    delete this;
                }
            }

            void onFinished(bool) { delete this; }

            AsyncClientStreamCallHandler* _owner;

            grpc::ServerContext _context;
            StreamReader _requestStreamReader;
        };

        void destroyHandlers()
        {
            auto it = _handlers.begin();
            while (it != _handlers.end())
            {
                delete *it;
                it = _handlers.erase(it);
            }
        }

        std::unordered_set<Handler*> _handlers;
    };
}
