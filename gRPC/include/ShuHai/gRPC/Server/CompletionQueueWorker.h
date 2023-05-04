#pragma once

#include "ShuHai/gRPC/Server/Detail/AsyncUnaryCallHandler.h"
#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/CompletionQueueNotification.h"

#include <grpcpp/grpcpp.h>

#include <unordered_set>
#include <utility>

namespace ShuHai::gRPC::Server
{
    /**
     * \brief grpc::ServerCompletionQueue wrapper for handling arbitrary client calls.
     */
    class CompletionQueueWorker
    {
    public:
        explicit CompletionQueueWorker(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : _queue(std::move(queue))
        { }

        ~CompletionQueueWorker() { deleteAllCallHandlers(); }

        CompletionQueueWorker(const CompletionQueueWorker&) = delete;
        CompletionQueueWorker& operator=(const CompletionQueueWorker&) = delete;

        /**
         * \brief Register certain rpc call handler corresponding to the specified function AsyncService::Request<RpcName>
         *  located in the generated code.
         * \param service Instance of the generated AsyncService class.
         * \param requestFunc Function address of AsyncService::Request<RpcName> located in generated code.
         * \param processFunc The function actually take care of the rpc newCall.
         */
        template<typename RequestFunc>
        void registerCallHandler(RequestFunc requestFunc,
            typename AsyncRequestTraits<RequestFunc>::ServiceType* service,
            std::function<void(grpc::ServerContext&, const typename AsyncRequestTraits<RequestFunc>::RequestType&,
                typename AsyncRequestTraits<RequestFunc>::ResponseType&)>
                processFunc)
        {
            static_assert(std::is_member_function_pointer_v<RequestFunc>);
            newUnaryCallHandler(requestFunc, service, std::move(processFunc));
        }

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

        [[nodiscard]] grpc::ServerCompletionQueue* underlyingQueue() const { return _queue.get(); }

    private:
        static void notifyComplete(void* tag, bool ok)
        {
            auto n = static_cast<CqNotification*>(tag);
            n->complete(ok);
            delete n;
        }

        std::unique_ptr<grpc::ServerCompletionQueue> _queue;


        // CallHandlers ------------------------------------------------------------------------------------------------
    private:
        using CallHandlerSet = std::unordered_set<Detail::AsyncCallHandlerBase*>;

        template<typename RequestFunc>
        auto newUnaryCallHandler(RequestFunc requestFunc,
            typename AsyncRequestTraits<RequestFunc>::ServiceType* service,
            std::function<void(grpc::ServerContext&, const typename AsyncRequestTraits<RequestFunc>::RequestType&,
                typename AsyncRequestTraits<RequestFunc>::ResponseType&)>
                processFunc)
        {
            auto handler = new Detail::AsyncUnaryCallHandler<RequestFunc>(
                _queue.get(), service, requestFunc, std::move(processFunc));
            _callHandlers.emplace(handler);
            return handler;
        }

        void deleteAllCallHandlers()
        {
            auto it = _callHandlers.begin();
            while (it != _callHandlers.end())
                it = deleteCallHandler(it);
        }

        auto deleteCallHandler(CallHandlerSet::iterator it) -> CallHandlerSet::iterator
        {
            assert(it != _callHandlers.end());

            auto handler = *it;
            it = _callHandlers.erase(it);
            delete handler;
            return it;
        }

        CallHandlerSet _callHandlers;
    };
}
