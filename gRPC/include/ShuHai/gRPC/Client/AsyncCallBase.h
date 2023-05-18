#pragma once

#include "ShuHai/gRPC/Client/TypeTraits.h"

#include <grpcpp/client_context.h>

#include <memory>

namespace ShuHai::gRPC::Client
{
    class AsyncCallBase
    {
    public:
        explicit AsyncCallBase(std::unique_ptr<grpc::ClientContext> context)
            : _context(std::move(context))
        {
            if (!_context)
                _context = std::make_unique<grpc::ClientContext>();
        }

        virtual ~AsyncCallBase() = default;

        grpc::ClientContext& context() { return *_context; }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

    protected:
        std::unique_ptr<grpc::ClientContext> _context;
        grpc::Status _status;
    };

    template<typename TCallFunc>
    class AsyncCall : public AsyncCallBase
    {
    public:
        using CallFunc = TCallFunc;
        using Stub = typename AsyncCallTraits<TCallFunc>::StubType;
        using Request = typename AsyncCallTraits<TCallFunc>::RequestType;
        using Response = typename AsyncCallTraits<TCallFunc>::ResponseType;
        using ResultCallback = std::function<void(std::future<Response>&&)>;
        using StreamingInterfaceType = typename AsyncCallTraits<TCallFunc>::StreamingInterfaceType;

        explicit AsyncCall(std::unique_ptr<grpc::ClientContext> context)
            : AsyncCallBase(std::move(context))
        { }
    };
}
