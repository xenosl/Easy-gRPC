#pragma once


namespace ShuHai::gRPC::Client::Detail
{
    class AsyncCallBase
    {
    public:
        virtual ~AsyncCallBase() = default;
        virtual void finish() = 0;
        virtual void throws() = 0;
    };
}
