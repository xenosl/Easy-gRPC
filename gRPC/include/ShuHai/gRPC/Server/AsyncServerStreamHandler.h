#pragma once

#include "ShuHai/gRPC/Server/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/CompletionQueueNotification.h"

#include <atomic>
#include <mutex>
#include <unordered_set>
#include <queue>
#include <optional>

namespace ShuHai::gRPC::Server
{
    template<typename TRequestFunc>
    class AsyncServerStreamHandler;

    enum class AsyncServerStreamState
    {
        ReadyWrite,
        Writing,
        WriteDone,
        Finish
    };

    template<typename TRequestFunc>
    class AsyncServerStreamWriter
    {
    public:
        using RequestFunc = TRequestFunc;
        using Response = typename AsyncRequestTraits<TRequestFunc>::ResponseType;

        void write(Response message, grpc::WriteOptions options = {})
        {
            auto state = _state.load();
            switch (state)
            {
            case AsyncServerStreamState::ReadyWrite:
                writeImpl(message, options);
                break;
            case AsyncServerStreamState::Writing:
                appendResponse(std::move(message), options);
                break;
            case AsyncServerStreamState::WriteDone:
                throw std::logic_error("Last message is written, no more message is allowed.");
            case AsyncServerStreamState::Finish:
                throw std::logic_error("Attempt to write to a finished stream.");
            }
        }

        void finish()
        {
            if (_state == AsyncServerStreamState::Finish)
                return;

            if (_state == AsyncServerStreamState::Writing)
                _pendingFinish = true;
            else
                finishImpl();
        }

        [[nodiscard]] AsyncServerStreamState state() const { return _state; }

        AsyncServerStreamWriter(grpc::ServerAsyncWriter<Response>& writer, std::function<void(bool)> onFinished)
            : _writer(writer)
            , _onFinished(std::move(onFinished))
        { }

    private:
        struct PendingResponse
        {
            Response message;
            grpc::WriteOptions options;
        };

        void appendResponse(Response message, grpc::WriteOptions options)
        {
            std::lock_guard l(_pendingResponsesMutex);

            if (!_pendingResponses.empty())
            {
                const auto& last = _pendingResponses.back();
                if (last.options.is_last_message())
                    throw std::logic_error("Last message is pended, no more message is allowed.");
            }

            _pendingResponses.emplace(PendingResponse { std::move(message), options });
        }

        std::optional<PendingResponse> takePendingResponse()
        {
            std::lock_guard l(_pendingResponsesMutex);

            if (_pendingResponses.empty())
                return std::nullopt;

            auto pr = std::move(_pendingResponses.front());
            _pendingResponses.pop();
            return std::move(pr);
        }

        void writeImpl(const PendingResponse& response) { writeImpl(response.message, response.options); }

        void writeImpl(const Response& message, grpc::WriteOptions options)
        {
            assert(_state == AsyncServerStreamState::ReadyWrite);

            _state = AsyncServerStreamState::Writing;
            _writer.Write(message, options,
                new GcqNotification([this, isLast = options.is_last_message()](bool ok) { onWritten(ok, isLast); }));
        }

        void finishImpl()
        {
            assert(_state != AsyncServerStreamState::Finish);

            _state = AsyncServerStreamState::Finish;
            _writer.Finish(grpc::Status::OK, new GcqNotification([this](bool ok) { onFinished(ok); }));
        }

        void onWritten(bool ok, bool isLast)
        {
            if (ok)
            {
                _state = isLast ? AsyncServerStreamState::WriteDone : AsyncServerStreamState::ReadyWrite;

                auto pendingResponse = takePendingResponse();
                if (pendingResponse)
                    writeImpl(pendingResponse.value());
                else if (_pendingFinish)
                    finishImpl();
            }
            else
            {
                finishImpl();
            }
        }

        void onFinished(bool ok)
        {
            _state = AsyncServerStreamState::Finish;

            _onFinished(ok);
        }

        std::atomic<AsyncServerStreamState> _state { AsyncServerStreamState::ReadyWrite };

        std::mutex _pendingResponsesMutex;
        std::queue<PendingResponse> _pendingResponses;

        std::atomic_bool _pendingFinish { false };

        grpc::ServerAsyncWriter<Response>& _writer;

        std::function<void(bool)> _onFinished;
    };

    template<typename TRequestFunc>
    class AsyncServerStreamHandler : public AsyncCallHandler<TRequestFunc>
    {
    public:
        using RequestFunc = TRequestFunc;
        using Base = AsyncCallHandler<RequestFunc>;
        using Service = typename Base::Service;
        using Request = typename Base::Request;
        using Response = typename Base::Response;
        using ServerStreamWriter = AsyncServerStreamWriter<RequestFunc>;
        using ProcessFunc = std::function<void(grpc::ServerContext&, const Request&, ServerStreamWriter&)>;

        explicit AsyncServerStreamHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc)
            : AsyncCallHandler<TRequestFunc>(completionQueue, service, requestFunc)
            , _processFunc(std::move(processFunc))
        {
            new Handler(this);
        }

        ~AsyncServerStreamHandler() { destroyHandlers(); }

    private:
        using ResponseWriter = typename Base::ResponseWriter;

        static_assert(std::is_same_v<grpc::ServerAsyncWriter<Response>, ResponseWriter>);

        ProcessFunc _processFunc;


    private:
        class Handler
        {
        public:
            explicit Handler(AsyncServerStreamHandler* owner)
                : _owner(owner)
                , _rawWriter(&_context)
                , _serverStreamWriter(_rawWriter, [this](bool ok) { onStreamWriterFinished(ok); })
            {
                auto cq = _owner->_completionQueue;
                auto service = _owner->_service;
                auto requestFunc = _owner->_requestFunc;

                (service->*requestFunc)(&_context, &_request, &_rawWriter, cq, cq,
                    new GcqNotification([&](bool ok) { onRequestResult(ok); }));

                _owner->_handlers.emplace(this);
            }

            ~Handler() { _owner->_handlers.erase(this); }

        private:
            void onRequestResult(bool ok)
            {
                if (ok)
                {
                    new Handler(_owner);
                    _owner->_processFunc(_context, _request, _serverStreamWriter);
                }
                else // Server shutdown
                {
                    delete this;
                }
            }

            void onStreamWriterFinished(bool ok) { delete this; }

            AsyncServerStreamHandler* const _owner;

            grpc::ServerContext _context;

            Request _request;

            ResponseWriter _rawWriter;
            ServerStreamWriter _serverStreamWriter;
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
