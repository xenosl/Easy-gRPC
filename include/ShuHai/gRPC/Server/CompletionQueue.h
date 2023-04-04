#pragma once

#include "ShuHai/gRPC/Server/Detail/AsyncCallHandlerBase.h"

#include <grpcpp/grpcpp.h>

#include <utility>

namespace ShuHai::gRPC::Server
{
    class CompletionQueue
    {
    public:
        explicit CompletionQueue(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : _queue(std::move(queue))
        { }

        CompletionQueue(const CompletionQueue&) = delete;
        CompletionQueue& operator=(const CompletionQueue&) = delete;

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
                handleCall(tag, ok);
                return true;
            case grpc::CompletionQueue::TIMEOUT:
                return true;
            default:
                throw std::runtime_error("Unsupported completion queue status.");
            }
        }

        [[nodiscard]] grpc::ServerCompletionQueue* queue() const { return _queue.get(); }

    private:
        void handleCall(void* tag, bool ok)
        {
            auto handler = static_cast<Detail::AsyncCallHandlerBase*>(tag);
            if (ok)
                handler->proceed(_queue.get());
            else
                handler->exit();
        }

        std::unique_ptr<grpc::ServerCompletionQueue> _queue;
    };
}
