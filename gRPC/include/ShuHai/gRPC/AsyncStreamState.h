#pragma once

namespace ShuHai::gRPC
{
    enum class AsyncStreamState
    {
        Ready,
        Streaming,
        Finished
    };
}
