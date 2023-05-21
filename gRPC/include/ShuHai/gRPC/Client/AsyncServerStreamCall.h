#pragma once

#include "ShuHai/gRPC/Client/AsyncCallBase.h"
#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/Client/RpcInvocationError.h"
#include "ShuHai/gRPC/CompletionQueueTag.h"
#include "ShuHai/gRPC/AsyncStreamState.h"

#include <grpcpp/support/async_stream.h>

#include <future>

namespace ShuHai::gRPC::Client
{
    template<typename TCallFunc>
    class AsyncServerStreamCall;

    template<typename TCallFunc>
    class AsyncResponseStreamReader
    {
    public:
        using CallFunc = TCallFunc;
        using Response = typename AsyncCallTraits<TCallFunc>::ResponseType;
        using ResponseReader = typename AsyncCallTraits<TCallFunc>::StreamingInterfaceType;

        static_assert(std::is_same_v<grpc::ClientAsyncReader<Response>, ResponseReader>);

        void moveNext(std::function<void(AsyncResponseStreamReader*)> onMoved)
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

        [[nodiscard]] AsyncStreamState state() const { return _state; }

        [[nodiscard]] bool finished() const { return _state == AsyncStreamState::Finished; }

    private:
        friend class AsyncServerStreamCall<CallFunc>;

        AsyncResponseStreamReader(
            std::unique_ptr<ResponseReader> reader, grpc::Status& status, std::function<void()> onFinished)
            : _reader(std::move(reader))
            , _status(status)
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
            _reader->Read(&_current, new GenericCompletionQueueTag([this](bool ok) { onRead(ok); }));
        }

        void finish(bool notify = true)
        {
            _state = AsyncStreamState::Finished;
            auto tag = notify
                ? static_cast<CompletionQueueTag*>(new GenericCompletionQueueTag([this](bool ok) { onFinished(ok); }))
                : static_cast<CompletionQueueTag*>(new DummyCompletionQueueTag());
            _reader->Finish(&_status, tag);
        }

        void ensureMoveNext()
        {
            if (_state == AsyncStreamState::Streaming)
                throw std::logic_error("Attempt to move next while the iterator is moving next.");
            if (_state == AsyncStreamState::Finished)
                throw std::logic_error("Attempt to move next after the iterator is finished.");
        }

        void onRead(bool ok)
        {
            _currentReadyPromise.set_value(ok);
            _currentReadyPromise = {};

            if (ok)
                _state.store(AsyncStreamState::Ready, std::memory_order_release);
            else
                finish();

            if (_onMovedNext)
                _onMovedNext(this);
        }

        void onFinished(bool ok) { assert(ok); }

        std::promise<bool> _currentReadyPromise;
        Response _current;
        std::function<void(AsyncResponseStreamReader*)> _onMovedNext;
        std::atomic<AsyncStreamState> _state { AsyncStreamState::Ready };

        grpc::Status& _status;

        std::unique_ptr<ResponseReader> _reader;

        std::function<void()> _onFinished;
    };

    template<typename TCallFunc>
    class AsyncServerStreamCall
        : public AsyncCall<TCallFunc>
        , public std::enable_shared_from_this<AsyncServerStreamCall<TCallFunc>>
    {
    public:
        using Base = AsyncCall<TCallFunc>;
        using CallFunc = typename Base::CallFunc;
        using Stub = typename Base::Stub;
        using Request = typename Base::Request;
        using Response = typename Base::Response;
        using ResponseStream = AsyncResponseStreamReader<TCallFunc>;

        AsyncServerStreamCall(Stub* stub, CallFunc asyncCall, std::unique_ptr<grpc::ClientContext> context,
            const Request& request, grpc::CompletionQueue* queue,
            std::function<void(std::shared_ptr<AsyncServerStreamCall>)> onFinished)
            : Base(std::move(context))
            , _onFinished(std::move(onFinished))
        {
            auto reader = (stub->*asyncCall)(this->_context.get(), request, queue,
                new GenericCompletionQueueTag([this](bool ok) { onReadyRead(ok); }));
            _responseStream =
                ResponseStreamPtr(new ResponseStream(std::move(reader), this->_status, [this] { this->onFinished(); }));
        }

        ~AsyncServerStreamCall() override = default;

        std::future<ResponseStream*> responseStream() { return _responseStreamPromise.get_future(); }

        [[nodiscard]] bool finished() const { return _responseStream->state() == AsyncStreamState::Finished; }

    private:
        struct ResponseStreamDeleter
        {
            void operator()(ResponseStream* p) const { delete p; }
        };

        using ResponseStreamPtr = std::unique_ptr<ResponseStream, ResponseStreamDeleter>;

        void onReadyRead(bool ok)
        {
            try
            {
                if (!ok)
                {
                    _responseStream->finish();
                    throw RpcInvocationError(RpcType::SERVER_STREAMING);
                }

                _responseStream->prepare();

                _responseStreamPromise.set_value(_responseStream.get());
            }
            catch (...)
            {
                _responseStreamPromise.set_exception(std::current_exception());
            }
        }

        void onFinished() { _onFinished(this->shared_from_this()); }

        ResponseStreamPtr _responseStream;
        std::promise<ResponseStream*> _responseStreamPromise;

        std::function<void(std::shared_ptr<AsyncServerStreamCall>)> _onFinished;
    };
}
