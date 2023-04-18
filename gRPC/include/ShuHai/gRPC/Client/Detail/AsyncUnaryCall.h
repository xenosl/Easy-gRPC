#pragma once

#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/Client/Exceptions.h"
#include "ShuHai/gRPC/Client/Detail/AsyncCallBase.h"

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <future>
#include <functional>

namespace ShuHai::gRPC::Client::Detail
{
    template<typename TPrepareFunc>
    class AsyncUnaryCall : public AsyncCallBase
    {
    public:
        using PrepareFunc = TPrepareFunc;
        using Stub = typename AsyncCallTraits<TPrepareFunc>::StubType;
        using Request = typename AsyncCallTraits<TPrepareFunc>::RequestType;
        using Response = typename AsyncCallTraits<TPrepareFunc>::ResponseType;
        using ResultCallback = std::function<void(std::future<Response>&&)>;

        static_assert(std::is_base_of_v<google::protobuf::Message, Request>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Response>);

        AsyncUnaryCall() = default;
        ~AsyncUnaryCall() override = default;

        std::future<Response>
        start(Stub* stub, PrepareFunc prepareFunc, const Request& request, grpc::CompletionQueue* queue)
        {
            startImpl(stub, prepareFunc, request, queue);
            return _resultPromise.get_future();
        }

        void start(Stub* stub, PrepareFunc prepareFunc, const Request& request, grpc::CompletionQueue* queue,
            ResultCallback callback)
        {
            startImpl(stub, prepareFunc, request, queue);
            _resultCallback = std::move(callback);
        }

        void finish() override
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

        void throws() override
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

        [[nodiscard]] const Response& response() const { return _response; }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

    private:
        void startImpl(Stub* stub, PrepareFunc prepareFunc, const Request& request, grpc::CompletionQueue* queue)
        {
            _responseReader = (stub->*prepareFunc)(&context(), request, queue);
            _responseReader->StartCall();
            _responseReader->Finish(&_response, &_status, this);
        }

        std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> _responseReader;
        Response _response;
        grpc::Status _status;

        std::promise<Response> _resultPromise;
        ResultCallback _resultCallback;
    };
}
