#pragma once

#include "ShuHai/gRPC/CompletionQueueNotification.h"

#include <grpcpp/completion_queue.h>

#include <utility>
#include <type_traits>

namespace ShuHai::gRPC
{
    /**
     * \brief grpc::CompletionQueue wrapper for its polling and event handling.
     *  Note: The class con only handle events tagged by CompletionQueueNotification.
     */
    template<typename Queue>
    class CompletionQueueWorker
    {
    public:
        static_assert(std::is_base_of_v<grpc::CompletionQueue, Queue>);

        explicit CompletionQueueWorker(std::unique_ptr<Queue> queue)
            : _queue(std::move(queue))
        { }

        virtual ~CompletionQueueWorker() = default;

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
            switch (status)
            {
            case grpc::CompletionQueue::SHUTDOWN:
                return false;
            case grpc::CompletionQueue::GOT_EVENT:
                notifyComplete(tag, ok);
                return true;
            case grpc::CompletionQueue::TIMEOUT:
                return true;
            default:
                throw std::runtime_error("Unsupported completion queue status.");
            }
        }

        void shutdown() { _queue->Shutdown(); }

        [[nodiscard]] Queue* queue() const { return _queue.get(); }

    private:
        static void notifyComplete(void* tag, bool ok)
        {
            auto n = static_cast<CqNotification*>(tag);
            n->complete(ok);
            delete n;
        }

        std::unique_ptr<Queue> _queue;
    };
}
