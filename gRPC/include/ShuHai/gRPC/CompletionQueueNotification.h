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

    /**
     * \brief CompletionQueueNotification that do nothing when complete.
     */
    class DummyCompletionQueueNotification : public CompletionQueueNotification
    {
    public:
        void complete(bool ok) override { }
    };

    using DcqNotification = DummyCompletionQueueNotification;

    /**
     * \brief Generic function wrapper for CompletionQueueNotification.
     */
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
