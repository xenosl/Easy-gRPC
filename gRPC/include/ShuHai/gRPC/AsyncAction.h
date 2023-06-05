#pragma once

#include <functional>

namespace ShuHai::gRPC
{
    class AsyncAction
    {
    public:
        explicit AsyncAction(std::function<void(bool)> finalizer = nullptr)
            : _finalizer(std::move(finalizer))
        { }

        virtual ~AsyncAction() = default;

        void finalizeResult(bool ok)
        {
            if (_finalizer)
                _finalizer(ok);
        }

    private:
        std::function<void(bool ok)> _finalizer;
    };
}
