#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/Server/Detail/AsyncCallHandlerBase.h"

#include <ShuHai/FunctionTraits.h>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <stdexcept>
#include <type_traits>


namespace ShuHai::gRPC::Server::Detail
{
    enum class AsyncUnaryCallStatus
    {
        Create,
        Process,
        Finish
    };

    template<typename TRequestFunc>
    class AsyncUnaryCallHandler : public AsyncCallHandlerBase
    {
    public:
        using RequestFunc = TRequestFunc;
        using Service = typename AsyncRequestFuncTraits<TRequestFunc>::ServiceType;
        using Request = typename AsyncRequestFuncTraits<TRequestFunc>::RequestType;
        using Response = typename AsyncRequestFuncTraits<TRequestFunc>::ResponseType;
        using ProcessFunc = std::function<void(const Request&, Response&)>;

        static_assert(std::is_base_of_v<grpc::Service, Service>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Request>);
        static_assert(std::is_base_of_v<google::protobuf::Message, Response>);

        static AsyncUnaryCallHandler* create(Service* service,
            RequestFunc requestFunc, ProcessFunc processFunc, grpc::ServerCompletionQueue* queue)
        {
            auto handler = new AsyncUnaryCallHandler(service, requestFunc, std::move(processFunc));
            handler->proceed(queue);
            return handler;
        }

        void proceed(grpc::ServerCompletionQueue* queue) override
        {
            if (_status == AsyncUnaryCallStatus::Create)
            {
                _status = AsyncUnaryCallStatus::Process;
                (_service->*_requestFunc)(&_context, &_request, &_responseWriter, queue, queue, this);
            }
            else if (_status == AsyncUnaryCallStatus::Process)
            {
                create(_service, _requestFunc, _processFunc, queue);

                _processFunc(_request, _response);

                _status = AsyncUnaryCallStatus::Finish;
                _responseWriter.Finish(_response, grpc::Status::OK, this);
            }
            else
            {
                assert(_status == AsyncUnaryCallStatus::Finish);
                delete this;
            }
        }

        void exit() override
        {
            delete this;
        }

    private:
        AsyncUnaryCallHandler(Service* service, RequestFunc requestFunc, ProcessFunc processFunc)
            : _service(service), _requestFunc(requestFunc), _processFunc(std::move(processFunc))
            , _responseWriter(&_context) { }
        ~AsyncUnaryCallHandler() override = default;

        AsyncUnaryCallStatus _status = AsyncUnaryCallStatus::Create;

        Service* _service { };
        RequestFunc _requestFunc { };
        ProcessFunc _processFunc { };

        grpc::ServerContext _context;
        Request _request;
        Response _response;
        grpc::ServerAsyncResponseWriter<Response> _responseWriter;
    };
}
