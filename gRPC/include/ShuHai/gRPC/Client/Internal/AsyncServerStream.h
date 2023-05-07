#pragma once

#include "ShuHai/gRPC/Client/Internal/AsyncCallBase.h"
#include "ShuHai/gRPC/CompletionQueueNotification.h"

namespace ShuHai::gRPC::Client::Internal
{
    template<typename TPrepareFunc>
    class AsyncServerStream : public AsyncCallBase
    {
    public:
    };
}
