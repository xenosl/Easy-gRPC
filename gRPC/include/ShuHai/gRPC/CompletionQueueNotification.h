#pragma once

#include <functional>
#include <stdexcept>

namespace ShuHai::gRPC
{
    class CompletionQueueNotification
    {
    public:
        virtual ~CompletionQueueNotification() = default;

        virtual void complete(bool ok) = 0;

        void* userObject {};
    };

    using CqNotification = CompletionQueueNotification;

    class GenericCompletionQueueNotification : public CompletionQueueNotification
    {
    public:
        explicit GenericCompletionQueueNotification(std::function<void(bool)> complete)
            : _complete(std::move(complete))
        { }

        void complete(bool ok) override
        {
            if (_complete)
                _complete(ok);
        }

    private:
        std::function<void(bool)> _complete;
    };

    using GcqNotification = GenericCompletionQueueNotification;
}
