#pragma once

#include <thread>

namespace ShuHai::gRPC::Examples
{
    inline void waitFor(int milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
}
