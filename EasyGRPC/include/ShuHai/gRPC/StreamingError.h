#pragma once

#include <stdexcept>

namespace ShuHai::gRPC
{
    class InvalidStreamingAction : public std::logic_error
    {
    public:
        explicit InvalidStreamingAction(const char* message)
            : std::logic_error(message)
        { }
    };
}
