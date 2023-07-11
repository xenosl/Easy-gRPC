#pragma once

#include "ShuHai/gRPC/AsyncAction.h"

#include <grpcpp/completion_queue.h>

#include <atomic>
#include <utility>
#include <type_traits>

namespace ShuHai::gRPC
{
    /**
     * \brief grpc::CompletionQueue wrapper for its polling and event handling.
     *  Note: The class con only handle events tagged by CompletionQueueTag.
     */
    class CompletionQueueWorker
    {
    public:
        explicit CompletionQueueWorker(std::unique_ptr<grpc::CompletionQueue> queue)
            : _queue(std::move(queue))
        { }

        virtual ~CompletionQueueWorker() { waitForShutdown(); }

        CompletionQueueWorker(const CompletionQueueWorker&) = delete;
        CompletionQueueWorker& operator=(const CompletionQueueWorker&) = delete;

        /**
         * \brief Block current thread up to the specified \p deadline or any queue event arrives.
         * \param deadline How long to block in wait for an event.
         * \return true if an event is handled or the \p deadline is up, false if the server shutdown.
         */
        bool poll(const gpr_timespec& deadline = gpr_inf_future(GPR_CLOCK_REALTIME))
        {
            void* tag {};
            bool ok;
            auto status = _queue->AsyncNext(&tag, &ok, deadline);
            bool nextPoll;
            switch (status)
            {
            case grpc::CompletionQueue::SHUTDOWN:
                nextPoll = false;
                break;
            case grpc::CompletionQueue::GOT_EVENT:
                finalizeResult(tag, ok);
                nextPoll = true;
                break;
            case grpc::CompletionQueue::TIMEOUT:
                nextPoll = true;
                break;
            default:
                throw std::runtime_error("Unsupported completion queue status.");
            }

            _pollStatus.store(status, std::memory_order_release);

            return nextPoll;
        }

        virtual void shutdown() { _queue->Shutdown(); }

        [[nodiscard]] grpc::CompletionQueue* queue() const { return _queue.get(); }

    protected:
        void waitForShutdown()
        {
            while (_pollStatus.load(std::memory_order_acquire) != grpc::CompletionQueue::SHUTDOWN)
                continue;
        }

    private:
        static void finalizeResult(void* tag, bool ok)
        {
            auto action = static_cast<IAsyncAction*>(tag);
            action->finalizeResult(ok);
            delete action;
        }

        std::unique_ptr<grpc::CompletionQueue> _queue;
        std::atomic<grpc::CompletionQueue::NextStatus> _pollStatus;
    };
}
