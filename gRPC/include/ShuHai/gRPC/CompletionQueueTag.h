#pragma once

#include <functional>
#include <stdexcept>

namespace ShuHai::gRPC
{
    class CompletionQueueTag
    {
    public:
        virtual ~CompletionQueueTag() = default;

        virtual void finalizeResult(bool ok) = 0;

        void* userObject {};
    };

    using CqTag = CompletionQueueTag;

    /**
     * \brief CompletionQueueTag that do nothing on finalize result.
     */
    class DummyCompletionQueueTag : public CompletionQueueTag
    {
    public:
        void finalizeResult(bool ok) override { }
    };

    using DcqTag = DummyCompletionQueueTag;

    /**
     * \brief Generic function wrapper for CompletionQueueTag.
     */
    class GenericCompletionQueueTag : public CompletionQueueTag
    {
    public:
        explicit GenericCompletionQueueTag(std::function<void(bool)> finalizer)
            : _finalizer(std::move(finalizer))
        { }

        void finalizeResult(bool ok) override
        {
            if (_finalizer)
                _finalizer(ok);
        }

    private:
        std::function<void(bool)> _finalizer;
    };

    using GcqTag = GenericCompletionQueueTag;
}
