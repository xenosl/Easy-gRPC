#pragma once

#include <grpcpp/grpcpp.h>

#include <stdexcept>

namespace ShuHai::gRPC::Client
{
    /**
     * \brief Throws when a call finished with error (i.e status is not ok).
     */
    class AsyncCallError : public std::runtime_error
    {
    public:
        explicit AsyncCallError(grpc::Status status)
            : std::runtime_error(status.error_message())
            , _status(std::move(status))
        { }

        [[nodiscard]] const grpc::Status& status() const { return _status; }

    private:
        grpc::Status _status;
    };
}
