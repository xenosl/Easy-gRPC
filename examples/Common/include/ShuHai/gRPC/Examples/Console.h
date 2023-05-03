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
        void writeLine(const char* message)
        {
            if (!_enabled)
                return;

            std::lock_guard l(_mutex);

            std::printf("%s\n", message);

            if (_flushImmediately)
                std::fflush(stdout);
        }

        template<typename... Args>
        void writeLine(const char* format, Args&&... args)
        {
            if (!_enabled)
                return;

            std::lock_guard l(_mutex);

            std::printf(format, std::forward<Args>(args)...);
            std::printf("\n");

            if (_flushImmediately)
                std::fflush(stdout);
        }

    private:
        bool _enabled = true;
        bool _flushImmediately = true;
        std::mutex _mutex;
    };

    inline Console& console()
    {
        return Console::instance();
    }
}
