#pragma once

#include "ShuHai/gRPC/Client/AsyncCallBase.h"
#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/Client/Exceptions.h"
#include "ShuHai/gRPC/CompletionQueueTag.h"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <future>
#include <functional>

namespace ShuHai::gRPC::Client
{
    template<typename TAsyncCall>
    class AsyncUnaryCall : public AsyncCallBase
    {
    public:
        using AsyncCall = TAsyncCall;
        using Stub = typename AsyncCallTraits<TAsyncCall>::StubType;
        using Request = typename AsyncCallTraits<TAsyncCall>::RequestType;
        using Response = typename AsyncCallTraits<TAsyncCall>::ResponseType;
        using ResultCallback = std::function<void(std::future<Response>&&)>;
        using ResponseReader = typename AsyncCallTraits<TAsyncCall>::ResponseReaderType;

        static_assert(std::is_base_of_v<google::protobuf::Message, Request>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Response>);
        static_assert(std::is_same_v<grpc::ClientAsyncResponseReader<Response>, ResponseReader>);

        AsyncUnaryCall() = default;
        ~AsyncUnaryCall() override = default;

        std::future<Response> invoke(
            Stub* stub, AsyncCall asyncCall, const Request& request, grpc::CompletionQueue* queue)
        {
            invokeImpl(stub, asyncCall, request, queue);
            return _resultPromise.get_future();
        }

        void invoke(Stub* stub, AsyncCall asyncCall, const Request& request, grpc::CompletionQueue* queue,
            ResultCallback callback)
        {
            invokeImpl(stub, asyncCall, request, queue);
            _resultCallback = std::move(callback);
        }

        [[nodiscard]] const Response& response() const { return _response; }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

    private:
        void invokeImpl(Stub* stub, AsyncCall asyncCall, const Request& request, grpc::CompletionQueue* queue)
        {
            _responseReader = (stub->*asyncCall)(&context, request, queue);
            _responseReader->Finish(&_response, &_status, new GcqTag([this](bool ok) { onFinished(ok); }));
        }

        void finish()
        {
            try
            {
                if (!_status.ok())
                    throw AsyncCallError(std::move(_status));

                _resultPromise.set_value(std::move(_response));
            }
            catch (...)
            {
                _resultPromise.set_exception(std::current_exception());
            }

            if (_resultCallback)
                _resultCallback(_resultPromise.get_future());
        }

        void throws()
        {
            try
            {
                throw AsyncQueueError();
            }
            catch (...)
            {
                _resultPromise.set_exception(std::current_exception());
            }

            if (_resultCallback)
                _resultCallback(_resultPromise.get_future());
        }

        void onFinished(bool ok)
        {
            if (ok)
                finish();
            else
                throws();

            delete this;
        }

        std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> _responseReader;
        Response _response;

        std::promise<Response> _resultPromise;
        ResultCallback _resultCallback;
    };
}
