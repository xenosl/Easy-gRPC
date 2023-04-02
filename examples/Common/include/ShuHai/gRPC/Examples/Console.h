#pragma once

#include <mutex>
#include <cstdio>

namespace ShuHai::gRPC::Examples
{
    class Console
    {
    public:
        static Console& instance()
        {
            static Console inst;
            return inst;
        }

        Console(const Console&) = delete;
        Console& operator=(const Console&) = delete;
        Console(Console&&) noexcept = delete;
        Console& operator=(Console&&) noexcept = delete;

    private:
        Console() = default;
        ~Console() = default;


    public:
        template<typename... Args>
        void writeLine(const char* format, Args&&... args)
        {
            std::lock_guard l(_mutex);
            std::printf(format, std::forward<Args>(args)...);
            std::printf("\n");
        }

    private:
        std::mutex _mutex;
    };

    inline Console& console()
    {
        return Console::instance();
    }
}
