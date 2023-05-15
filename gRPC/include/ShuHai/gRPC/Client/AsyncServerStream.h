#pragma once

#include "ShuHai/gRPC/Client/AsyncCallBase.h"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <mutex>
#include <condition_variable>

namespace ShuHai::gRPC::Client
{
    template<typename TAsyncCall>
    class AsyncServerStream
        : public AsyncCallBase
        , public std::enable_shared_from_this<AsyncServerStream<TAsyncCall>>
    {
    public:
        using AsyncCall = TAsyncCall;
        using Stub = typename AsyncCallTraits<TAsyncCall>::StubType;
        using Request = typename AsyncCallTraits<TAsyncCall>::RequestType;
        using Response = typename AsyncCallTraits<TAsyncCall>::ResponseType;
        using ResponseReader = typename AsyncCallTraits<TAsyncCall>::ResponseReaderType;
        using ResponseCallback = std::function<void(const Response&)>;

        static_assert(std::is_base_of_v<google::protobuf::Message, Request>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Response>);
        static_assert(std::is_same_v<grpc::ClientAsyncReader<Response>, ResponseReader>);

        AsyncServerStream(Stub* stub, AsyncCall asyncCall, const Request& request, grpc::CompletionQueue* queue,
            ResponseCallback responseCallback, std::function<void(std::shared_ptr<AsyncServerStream>)> finishCallback)
            : _responseCallback(std::move(responseCallback))
            , _finishCallback(std::move(finishCallback))
        {
            _streamReader =
                (stub->*asyncCall)(&context, request, queue, new GcqTag([this](bool ok) { onReadyRead(ok); }));
        }

        ~AsyncServerStream() override
        {
            if (!_finished)
                finish(false);
        }

        void waitResponses()
        {
            std::unique_lock l(_responsesMutex);
            _responsesCv.wait(l, [this] { return !_responses.empty(); });
        }

        std::vector<Response> takeResponses()
        {
            std::vector<Response> r;
            {
                std::lock_guard l(_responsesMutex);
                r = std::move(_responses);
            }
            _responsesCv.notify_all();
            return r;
        }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

        [[nodiscard]] bool finished() const { return _finished; }

    private:
        void read()
        {
            _streamReader->Read(&_readingResponse, new GcqTag([this](bool ok) { onRead(ok); }));
        }

        void finish(bool notify = true)
        {
            auto notification = notify ? static_cast<CqTag*>(new GcqTag([this](bool ok) { onFinished(ok); }))
                                       : static_cast<CqTag*>(new DcqTag());
            _streamReader->Finish(&_status, notification);
        }

        void onReadyRead(bool ok)
        {
            if (ok)
                read();
            else
                finish();
        }

        void onRead(bool ok)
        {
            if (ok)
            {
                if (_responseCallback)
                    _responseCallback(_readingResponse);

                {
                    std::lock_guard l(_responsesMutex);
                    _responses.emplace_back(std::move(_readingResponse));
                }
                _responsesCv.notify_all();

                read();
            }
            else
            {
                finish();
            }
        }

        void onFinished(bool ok)
        {
            _finished = true;

            _finishCallback(this->shared_from_this());
        }

        ResponseCallback _responseCallback;
        std::function<void(std::shared_ptr<AsyncServerStream>)> _finishCallback;

        std::mutex _responsesMutex;
        std::condition_variable _responsesCv;
        Response _readingResponse;
        std::vector<Response> _responses;

        std::atomic_bool _finished { false };
        std::unique_ptr<ResponseReader> _streamReader;
    };
}
