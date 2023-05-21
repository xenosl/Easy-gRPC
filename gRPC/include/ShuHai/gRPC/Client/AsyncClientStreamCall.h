#pragma once

#include "ShuHai/gRPC/Client/AsyncCallBase.h"
#include "ShuHai/gRPC/Client/TypeTraits.h"

#include <future>
#include <atomic>

namespace ShuHai::gRPC::Client
{
    template<typename TCallFunc>
    class AsyncClientStreamCall;

    template<typename TCallFunc>
    class AsyncRequestStreamWriter
    {
    public:
        using Request = typename AsyncCallTraits<TCallFunc>::RequestType;
        using RequestWriter = typename AsyncCallTraits<TCallFunc>::StreamingInterfaceType;

        static_assert(std::is_same_v<grpc::ClientAsyncWriter<Request>, RequestWriter>);

        std::future<bool> write(const Request& request, grpc::WriteOptions options = {})
        {
            auto state = _state.load(std::memory_order_acquire);
            if (state == AsyncStreamState::Streaming)
                throw std::logic_error("Attempt to write to the stream while its writing.");
            if (state == AsyncStreamState::Finished)
                throw std::logic_error("Attempt to write to the stream after its finished.");

            assert(state == AsyncStreamState::Ready);

            _state = AsyncStreamState::Streaming;
            _writer->Write(request, options, new GenericCompletionQueueTag([this](bool ok) { onWritten(ok); }));
            return _writePromise.get_future();
        }

        std::future<void> finish()
        {
            auto state = _state.load(std::memory_order_acquire);

            if (state == AsyncStreamState::Streaming)
                throw std::logic_error("Attempt to finish the stream while its writing.");

            if (state == AsyncStreamState::Finished)
            {
                std::promise<void> emptyPromise;
                emptyPromise.set_value();
                return emptyPromise.get_future();
            }

            assert(state == AsyncStreamState::Ready);

            _state = AsyncStreamState::Finished;
            _writer->Finish(&_status, new GenericCompletionQueueTag([this](bool ok) { onFinished(ok); }));
            return _finishPromise.get_future();
        }

    private:
        friend class AsyncClientStreamCall<TCallFunc>;

        AsyncRequestStreamWriter(
            std::unique_ptr<RequestWriter> writer, grpc::Status& status, std::function<void()> onFinished)
            : _writer(std::move(writer))
            , _status(status)
            , _onFinished(std::move(onFinished))
        { }

        void prepare()
        {
            _writePromise = {};
            _finishPromise = {};

            _state.store(AsyncStreamState::Ready, std::memory_order_release);
        }

        void onWritten(bool ok)
        {
            // ok means that the data/metadata/status/etc is going to go to thewire.
            // If it is false, it not going to the wire because the call is already dead (i.e., canceled, deadline
            // expired, other side dropped the channel, etc).

            _writePromise.set_value(ok);
            _writePromise = {};

            if (ok)
                _state.store(AsyncStreamState::Ready, std::memory_order_release);
            else
                finish();
        }

        void onFinished(bool ok)
        {
            assert(ok);

            try
            {
                _onFinished();

                if (!_status.ok())
                    throw AsyncCallError(_status);

                _finishPromise.set_value();
            }
            catch (...)
            {
                _finishPromise.set_exception(std::current_exception());
            }
        }

        std::promise<bool> _writePromise;
        std::promise<void> _finishPromise;

        std::atomic<AsyncStreamState> _state { AsyncStreamState::Ready };

        grpc::Status& _status;
        std::unique_ptr<RequestWriter> _writer;

        std::function<void()> _onFinished;
    };

    template<typename TCallFunc>
    class AsyncClientStreamCall
        : public AsyncCall<TCallFunc>
        , public std::enable_shared_from_this<AsyncClientStreamCall<TCallFunc>>
    {
    public:
        using Base = AsyncCall<TCallFunc>;
        using CallFunc = typename Base::CallFunc;
        using Stub = typename Base::Stub;
        using Request = typename Base::Request;
        using Response = typename Base::Response;
        using RequestStream = AsyncRequestStreamWriter<TCallFunc>;

        AsyncClientStreamCall(Stub* stub, CallFunc asyncCall, std::unique_ptr<grpc::ClientContext> context,
            grpc::CompletionQueue* queue, std::function<void(std::shared_ptr<AsyncClientStreamCall>)> onFinished)
            : Base(std::move(context))
            , _onFinished(std::move(onFinished))
        {
            auto writer = (stub->*asyncCall)(this->_context.get(), &_response, queue,
                new GenericCompletionQueueTag([this](bool ok) { onReadyWrite(ok); }));
            _requestStream =
                RequestStreamPtr(new RequestStream(std::move(writer), _status, [this]() { this->onFinished(); }));
        }

        std::future<RequestStream*> requestStream() { return _requestStreamPromise.get_future(); }

    private:
        struct RequestStreamDeleter
        {
            void operator()(RequestStream* p) const { delete p; }
        };

        using RequestStreamPtr = std::unique_ptr<RequestStream, RequestStreamDeleter>;

        void onReadyWrite(bool ok)
        {
            try
            {
                if (!ok)
                {
                    _requestStream->finish();
                    throw RpcInvocationError(RpcType::CLIENT_STREAMING);
                }

                _requestStream->prepare();

                _requestStreamPromise.set_value(_requestStream.get());
            }
            catch (...)
            {
                _requestStreamPromise.set_exception(std::current_exception());
            }
        }

        void onFinished() { _onFinished(this->shared_from_this()); }

        RequestStreamPtr _requestStream;
        std::promise<RequestStream*> _requestStreamPromise;

        Response _response;
        grpc::Status _status;

        std::function<void(std::shared_ptr<AsyncClientStreamCall>)> _onFinished;
    };
}
