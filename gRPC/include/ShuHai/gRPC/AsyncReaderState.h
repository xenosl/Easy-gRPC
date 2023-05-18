#pragma once

namespace ShuHai::gRPC
{
    enum class AsyncReaderState
    {
        ReadyRead,
        Reading,
        Finished
    };
}
