#pragma once

#include "ShuHai/gRPC/Client/AsyncStreamCall.h"
#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/Client/RpcInvocationError.h"
#include "ShuHai/gRPC/CompletionQueueTag.h"

#include <grpcpp/support/async_stream.h>
#include <google/protobuf/message.h>

#include <future>

namespace ShuHai::gRPC::Client
{
    template<typename TAsyncCall>
    class AsyncServerStreamCall;

    enum class AsyncResponseIteratorState
    {
        ReadyMoveNext,
        MovingNext,
        Finished
    };

    template<typename TAsyncCall>
    class AsyncResponseIterator
    {
    public:
        using AsyncCall = TAsyncCall;
        using Response = typename AsyncCallTraits<TAsyncCall>::ResponseType;
        using ResponseReader = typename AsyncCallTraits<TAsyncCall>::ResponseReaderType;

        static_assert(std::is_same_v<grpc::ClientAsyncReader<Response>, ResponseReader>);

        void moveNext(std::function<void(AsyncResponseIterator*)> onMoved)
        {
            ensureMoveNext();
            _onMovedNext = std::move(onMoved);
            read();
        }

        std::future<bool> moveNext()
        {
            ensureMoveNext();
            read();
            return _currentReadyPromise.get_future();
        }

        [[nodiscard]] const Response& current() const { return _current; }

        [[nodiscard]] AsyncResponseIteratorState state() const { return _state; }

        [[nodiscard]] bool finished() const { return _state == AsyncResponseIteratorState::Finished; }

    private:
        friend class AsyncServerStreamCall<AsyncCall>;

        AsyncResponseIterator(
            std::unique_ptr<ResponseReader> reader, grpc::Status& status, std::function<void()> onFinished)
            : _reader(std::move(reader))
            , _status(status)
            , _onFinished(std::move(onFinished))
        { }

        void prepare()
        {
            _currentReadyPromise = {};
            _state.store(AsyncResponseIteratorState::ReadyMoveNext, std::memory_order_release);
        }

        void read()
        {
            _state = AsyncResponseIteratorState::MovingNext;
            _reader->Read(&_current, new GenericCompletionQueueTag([this](bool ok) { onRead(ok); }));
        }

        void finish(bool notify = true)
        {
            _state = AsyncResponseIteratorState::Finished;
            auto tag = notify
                ? static_cast<CompletionQueueTag*>(new GenericCompletionQueueTag([this](bool ok) { onFinished(ok); }))
                : static_cast<CompletionQueueTag*>(new DummyCompletionQueueTag());
            _reader->Finish(&_status, tag);
        }

        void ensureMoveNext()
        {
            if (_state == AsyncResponseIteratorState::MovingNext)
                throw std::logic_error("Attempt to move next while the iterator is moving next.");
            if (_state == AsyncResponseIteratorState::Finished)
                throw std::logic_error("Attempt to move next after the iterator is finished.");
        }

        void onRead(bool ok)
        {
            _currentReadyPromise.set_value(ok);
            _currentReadyPromise = {};

            if (ok)
                _state.store(AsyncResponseIteratorState::ReadyMoveNext, std::memory_order_release);
            else
                finish();

            if (_onMovedNext)
                _onMovedNext(this);
        }

        void onFinished(bool ok) { assert(ok); }

        std::promise<bool> _currentReadyPromise;
        Response _current;
        std::function<void(AsyncResponseIterator*)> _onMovedNext;
        std::atomic<AsyncResponseIteratorState> _state { AsyncResponseIteratorState::ReadyMoveNext };

        grpc::Status& _status;

        std::unique_ptr<ResponseReader> _reader;

        std::function<void()> _onFinished;
    };

    template<typename TAsyncCall>
    class AsyncServerStreamCall
        : public AsyncStreamCall
        , public std::enable_shared_from_this<AsyncServerStreamCall<TAsyncCall>>
    {
    public:
        using AsyncCall = TAsyncCall;
        using Stub = typename AsyncCallTraits<TAsyncCall>::StubType;
        using Request = typename AsyncCallTraits<TAsyncCall>::RequestType;
        using Response = typename AsyncCallTraits<TAsyncCall>::ResponseType;
        using ResponseIterator = AsyncResponseIterator<AsyncCall>;

        static_assert(std::is_base_of_v<google::protobuf::Message, Request>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Response>);

        AsyncServerStreamCall(Stub* stub, AsyncCall asyncCall, std::unique_ptr<grpc::ClientContext> context,
            const Request& request, grpc::CompletionQueue* queue,
            std::function<void(std::shared_ptr<AsyncServerStreamCall>)> onFinished)
            : _context(std::move(context))
            , _onFinished(std::move(onFinished))
        {
            if (!_context)
                _context = std::make_unique<grpc::ClientContext>();

            auto reader = (stub->*asyncCall)(
                _context.get(), request, queue, new GenericCompletionQueueTag([this](bool ok) { onReadyRead(ok); }));
            _responseIterator =
                ResponseIteratorPtr(new ResponseIterator(std::move(reader), _status, [this] { this->onFinished(); }));
        }

        ~AsyncServerStreamCall() override = default;

        std::future<ResponseIterator*> responseIterator() { return _responseIteratorPromise.get_future(); }

        grpc::ClientContext& context() { return *_context; }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

        [[nodiscard]] bool finished() const
        {
            return _responseIterator->state() == AsyncResponseIteratorState::Finished;
        }

    private:
        struct ResponseIteratorDeleter
        {
            void operator()(ResponseIterator* p) const { delete p; }
        };

        using ResponseIteratorPtr = std::unique_ptr<ResponseIterator, ResponseIteratorDeleter>;

        void onReadyRead(bool ok)
        {
            try
            {
                if (!ok)
                {
                    _responseIterator->finish();
                    throw RpcInvocationError(RpcType::SERVER_STREAMING);
                }

                _responseIterator->prepare();

                _responseIteratorPromise.set_value(_responseIterator.get());
            }
            catch (...)
            {
                _responseIteratorPromise.set_exception(std::current_exception());
            }
        }

        void onFinished() { _onFinished(this->shared_from_this()); }

        std::unique_ptr<grpc::ClientContext> _context;
        grpc::Status _status;

        ResponseIteratorPtr _responseIterator;
        std::promise<ResponseIterator*> _responseIteratorPromise;

        std::function<void(std::shared_ptr<AsyncServerStreamCall>)> _onFinished;
    };
}
