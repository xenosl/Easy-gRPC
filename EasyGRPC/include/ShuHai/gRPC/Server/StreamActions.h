#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/AsyncAction.h"

namespace ShuHai::gRPC::Server
{
    template<typename Response>
    class UnaryCallFinishAction : public AsyncAction
    {
    public:
        using Stream = grpc::ServerAsyncResponseWriter<Response>;

        explicit UnaryCallFinishAction(Stream& stream, std::function<void(bool ok)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stream(stream)
        { }

        void perform(const Response& response, const grpc::Status& status) { _stream.Finish(response, status, this); }

    private:
        Stream& _stream;
    };

    template<typename Response>
    void unaryCallFinish(grpc::ServerAsyncResponseWriter<Response>& stream, const Response& response,
        const grpc::Status& status, std::function<void(bool ok)> finalizer = nullptr)
    {
        (new UnaryCallFinishAction(stream, std::move(finalizer)))->perform(response, status);
    }
}
