#pragma once

#include <grpcpp/client_context.h>

#include <memory>

namespace ShuHai::gRPC::Client
{
    class AsyncCall
    {
    public:
        explicit AsyncCall(std::unique_ptr<grpc::ClientContext> context)
            : _context(std::move(context))
        {
            if (!_context)
                _context = std::make_unique<grpc::ClientContext>();
        }

        virtual ~AsyncCall() = default;

        virtual void shutdown() { }

        grpc::ClientContext& context() { return *_context; }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

    protected:
        std::unique_ptr<grpc::ClientContext> _context;
        grpc::Status _status;
    };
}
