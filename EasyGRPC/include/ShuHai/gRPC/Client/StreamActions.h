#pragma once

#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/AsyncAction.h"

namespace ShuHai::gRPC::Client
{
    template<typename Response>
    class UnaryCallFinishAction : public AsyncAction
    {
    public:
        using Stream = grpc::ClientAsyncResponseReader<Response>;

        explicit UnaryCallFinishAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stream(stream)
        { }

        void perform(Response* response, grpc::Status* status) { _stream.Finish(response, status, this); }

    private:
        Stream& _stream;
    };

    template<typename Response>
    void unaryCallFinish(grpc::ClientAsyncResponseReader<Response>& stream, Response* response, grpc::Status* status,
        std::function<void(bool)> finalizer = nullptr)
    {
        (new UnaryCallFinishAction<Response>(stream, std::move(finalizer)))->perform(response, status);
    }

    /**
     * \brief Base class for async-actions on client-side of client streaming rpc.
     * \tparam Request Request type of the rpc.
     */
    template<typename Request>
    class ClientStreamAction : public AsyncAction
    {
    public:
        using Stream = grpc::ClientAsyncWriter<Request>;

        explicit ClientStreamAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stream(stream)
        { }

    protected:
        Stream& _stream;
    };

    template<typename Request>
    class ClientStreamWriteAction : public ClientStreamAction<Request>
    {
    public:
        using Stream = grpc::ClientAsyncWriter<Request>;

        explicit ClientStreamWriteAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : ClientStreamAction<Request>(stream, std::move(finalizer))
        { }

        void perform(const Request& request, grpc::WriteOptions options)
        {
            this->_stream.Write(request, options, this);
        }

        void perform(const Request& request) { this->_stream.Write(request, this); }
    };

    template<typename Request>
    class ClientStreamWritesDoneAction : public ClientStreamAction<Request>
    {
    public:
        using Stream = grpc::ClientAsyncWriter<Request>;

        explicit ClientStreamWritesDoneAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : ClientStreamAction<Request>(stream, std::move(finalizer))
        { }

        void perform() { this->_stream.WritesDone(this); }
    };

    template<typename Request>
    class ClientStreamFinishAction : public ClientStreamAction<Request>
    {
    public:
        using Stream = grpc::ClientAsyncWriter<Request>;

        explicit ClientStreamFinishAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : ClientStreamAction<Request>(stream, std::move(finalizer))
        { }

        void perform(grpc::Status* status) { this->_stream.Finish(status, this); }
    };

    /**
     * \brief Base class for async-actions on client-side of server streaming rpc.
     * \tparam Response Response type of the rpc.
     */
    template<typename Response>
    class ServerStreamAction : public AsyncAction
    {
    public:
        using Stream = grpc::ClientAsyncReader<Response>;

        explicit ServerStreamAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : AsyncAction(std::move(finalizer))
            , _stream(stream)
        { }

    protected:
        Stream& _stream;
    };

    template<typename Response>
    class ServerStreamReadAction : ServerStreamAction<Response>
    {
    public:
        using Stream = grpc::ClientAsyncReader<Response>;

        explicit ServerStreamReadAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : ServerStreamAction<Response>(stream, std::move(finalizer))
        { }

        void perform(Response* response) { this->_stream.Read(response, this); }
    };

    template<typename Response>
    class ServerStreamFinishAction : ServerStreamAction<Response>
    {
    public:
        using Stream = grpc::ClientAsyncReader<Response>;

        explicit ServerStreamFinishAction(Stream& stream, std::function<void(bool)> finalizer = nullptr)
            : ServerStreamAction<Response>(stream, std::move(finalizer))
        { }

        void perform(grpc::Status* status) { this->_stream.Finish(status, this); }
    };
}
