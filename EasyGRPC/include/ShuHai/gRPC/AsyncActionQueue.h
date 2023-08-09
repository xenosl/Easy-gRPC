#pragma once

#include "ShuHai/gRPC/IAsyncAction.h"

#include <grpcpp/completion_queue.h>

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <type_traits>

namespace ShuHai::gRPC
{
    /**
     * \brief grpc::CompletionQueue wrapper that only accepts IAsyncAction as its tag.
     */
    class AsyncActionQueue
    {
    public:
        explicit AsyncActionQueue(std::unique_ptr<grpc::CompletionQueue> cq)
            : _completionQueue(std::move(cq))
        { }

        virtual ~AsyncActionQueue() { waitForShutdown(); }

        AsyncActionQueue(const AsyncActionQueue&) = delete;
        AsyncActionQueue& operator=(const AsyncActionQueue&) = delete;

        /**
         * \brief Block current thread up to the specified \p deadline or any completionQueue event arrives.
         * \param deadline How long to block in wait for an event.
         * \return true if an event is handled or the \p deadline is up, false if the server shutdown.
         */
        grpc::CompletionQueue::NextStatus asyncNext(const gpr_timespec& deadline = gpr_inf_future(GPR_CLOCK_REALTIME))
        {
            void* tag {};
            bool ok;
            auto status = _completionQueue->AsyncNext(&tag, &ok, deadline);
            if (status == grpc::CompletionQueue::GOT_EVENT)
                finalizeResult(tag, ok);

            _nextStatus.store(status, std::memory_order_release);

            return status;
        }

        virtual void shutdown() { _completionQueue->Shutdown(); }

        /**
         * \brief The underlying grpc::CompletionQueue that current instance wraps.
         */
        [[nodiscard]] grpc::CompletionQueue* completionQueue() const { return _completionQueue.get(); }

    private:
        static void finalizeResult(void* tag, bool ok)
        {
            auto action = static_cast<IAsyncAction*>(tag);
            action->finalizeResult(ok);
            delete action;
        }

        void waitForShutdown()
        {
            while (_nextStatus.load(std::memory_order_acquire) != grpc::CompletionQueue::SHUTDOWN)
                continue;
        }

        std::unique_ptr<grpc::CompletionQueue> _completionQueue;

        std::atomic<grpc::CompletionQueue::NextStatus> _nextStatus;
    };
}
