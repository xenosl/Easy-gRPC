#pragma once

#include "ShuHai/gRPC/Server/Detail/AsyncUnaryCallHandler.h"
#include "ShuHai/gRPC/Server/TypeTraits.h"

#include <grpcpp/grpcpp.h>

#include <utility>

namespace ShuHai::gRPC::Server
{
    /**
     * \brief grpc::ServerCompletionQueue wrapper for handling arbitrary client calls.
     */
    class CompletionQueue
    {
    public:
        explicit CompletionQueue(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : _queue(std::move(queue))
        { }

        CompletionQueue(const CompletionQueue&) = delete;
        CompletionQueue& operator=(const CompletionQueue&) = delete;

        /**
         * \brief Register certain rpc call handler corresponding to the specified function AsyncService::Request<RpcName>
         *  located in the generated code.
         * \param service Instance of the generated AsyncService class.
         * \param requestFunc Function address of AsyncService::Request<RpcName> located in generated code.
         * \param processFunc The function actually take care of the rpc request.
         */
        template<typename RequestFunc>
        void registerCallHandler(RequestFunc requestFunc,
            typename AsyncRequestTraits<RequestFunc>::ServiceType* service,
            std::function<void(const typename AsyncRequestTraits<RequestFunc>::RequestType&,
                typename AsyncRequestTraits<RequestFunc>::ResponseType&)> processFunc)
        {
            Detail::AsyncUnaryCallHandler<RequestFunc>::create(service, requestFunc, std::move(processFunc), _queue.get());
        }

        /**
         * \brief Block current thread up to the specified \p deadline or any queue event arrives.
         * \param deadline How long to block in wait for an event.
         * \return true if an event is handled or the \p deadline is up, false if the server shutdown.
         */
        bool next(const gpr_timespec& deadline = gpr_inf_future(GPR_CLOCK_REALTIME))
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

        void shutdown() { _queue->Shutdown(); }

        [[nodiscard]] grpc::ServerCompletionQueue* underlyingQueue() const { return _queue.get(); }

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
