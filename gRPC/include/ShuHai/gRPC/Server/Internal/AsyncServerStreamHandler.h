#pragma once

#include "ShuHai/gRPC/Server/Internal/AsyncCallHandlerBase.h"
#include "ShuHai/gRPC/CompletionQueueNotification.h"

#include <condition_variable>
#include <unordered_map>

namespace ShuHai::gRPC::Server::Internal
{
    template<typename TRequestFunc>
    class AsyncServerStreamHandler;

    enum class AsyncStreamState
    {
        Ready,
        Streaming,
        StreamDone,
        Finished
    };

    template<typename TRequestFunc>
    class ServerStreamWriter
    {
    public:
        using RequestFunc = TRequestFunc;
        using Response = typename AsyncRequestTraits<TRequestFunc>::ResponseType;

        void write(const Response& message, grpc::WriteOptions options)
        {
            if (_state == AsyncStreamState::StreamDone)
                throw std::logic_error("Last message already written, no more message is allowed.");
            if (_state == AsyncStreamState::Finished)
                throw std::logic_error("Stream finished, no more message is allowed.");

            if (_state == AsyncStreamState::Ready)
                _state = AsyncStreamState::Streaming;

            assert(_state == AsyncStreamState::Streaming);
            if (options.is_last_message())
                _state = AsyncStreamState::StreamDone;

            waitForWriterReady();
            _writerReady = false;
            _writer.Write(message, options, new GcqNotification([this](bool ok) { onWritten(ok); }));
        }

        void finish()
        {
            if (_state == AsyncStreamState::Finished)
                return;

            _state = AsyncStreamState::Finished;

            waitForWriterReady();
            _writerReady = false;
            _writer.Finish(grpc::Status::OK, new GcqNotification([this](bool ok) { onFinished(ok); }));
        }

        [[nodiscard]] AsyncStreamState state() const { return _state; }

    private:
        friend class AsyncServerStreamHandler<RequestFunc>;
        using Callback = std::function<void(bool)>;

        explicit ServerStreamWriter(grpc::ServerContext* context, Callback onWriterNotification)
            : _writer(context)
            , _onWriterNotify(std::move(onWriterNotification))
        { }

        void waitForWriterReady()
        {
            std::unique_lock l(_writerMutex);
            _writerCv.wait(l, [this] { return _writerReady; });
        }

        void markWriterReady()
        {
            _writerReady = true;
            _writerCv.notify_one();
        }

        void onWritten(bool ok)
        {
            markWriterReady();
            _onWriterNotify(ok);
        }

        void onFinished(bool ok)
        {
            markWriterReady();
            _onWriterNotify(ok);
        }

        bool _writerReady = true;
        std::mutex _writerMutex;
        std::condition_variable _writerCv;

        grpc::ServerAsyncWriter<Response> _writer;

        std::atomic<AsyncStreamState> _state { AsyncStreamState::Ready };

        Callback _onWriterNotify;
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
        using ResponseWriter = typename Base::ResponseWriter;
        using StreamWriter = ServerStreamWriter<RequestFunc>;
        using ProcessFunc = std::function<void(grpc::ServerContext&, const Request&, StreamWriter&)>;

        explicit AsyncServerStreamHandler(grpc::ServerCompletionQueue* completionQueue, Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc)
            : AsyncCallHandler<TRequestFunc>(completionQueue, service, requestFunc)
            , _processFunc(std::move(processFunc))
        {
            newHandler();
        }

        ~AsyncServerStreamHandler() { deleteAllHandlers(); }

    private:
        ProcessFunc _processFunc;


    private:
        class Handler;
        using HandlerCallback = void (AsyncServerStreamHandler::*)(Handler*, bool);

        class Handler
        {
        public:
            explicit Handler(AsyncServerStreamHandler* owner, HandlerCallback onProcess, HandlerCallback onFinish)
                : _owner(owner)
                //, _onProcess(onProcess)
                //, _onFinish(onFinish)
                , _streamWriter(&_context, [this](bool ok) { onStreamWriterNotify(ok); })
            { }

            void request()
            {
                auto cq = _owner->_completionQueue;
                auto service = _owner->_service;
                auto requestFunc = _owner->_requestFunc;

                (service->*requestFunc)(&_context, &_request, &_streamWriter._writer, cq, cq,
                    new GcqNotification([&](bool ok) { process(ok); }));
            }

        private:
            void process(bool ok)
            {
                (_owner->*_onProcess)(this, ok);

                if (!ok)
                    return;

                _owner->_processFunc(_context, _request, _streamWriter);
            }

            void onStreamWriterNotify(bool ok)
            {
                if (ok)
                {
                    auto state = _streamWriter.state();
                    if (state == StreamWriter::AsyncStreamState::Finished)
                        _owner->deleteHandler(this);
                }
                else
                {
                    _owner->deleteHandler(this);
                }
            }

            //void finish(bool ok) { (_owner->*_onFinish)(this, ok); }

            AsyncServerStreamHandler* const _owner;
            //HandlerCallback _onProcess;
            //HandlerCallback _onFinish;

            grpc::ServerContext _context;
            StreamWriter _streamWriter;
            Request _request;
        };

        void newHandler()
        {
            auto handler = new Handler(
                this, &AsyncServerStreamHandler::onHandlerProcess, &AsyncServerStreamHandler::onHandlerFinish);
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

        //void onHandlerFinish(Handler* handler, bool ok) { deleteHandler(handler); }

        std::unordered_set<Handler*> _handlers;
    };
}
