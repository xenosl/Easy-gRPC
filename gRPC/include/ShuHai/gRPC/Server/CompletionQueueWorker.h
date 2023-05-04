#pragma once

#include "ShuHai/gRPC/Server/Detail/AsyncUnaryCallHandler.h"
#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/CompletionQueueWorker.h"

#include <unordered_set>
#include <utility>

namespace ShuHai::gRPC::Server
{
    class CompletionQueueWorker : public gRPC::CompletionQueueWorker<grpc::ServerCompletionQueue>
    {
    public:
        using Base = gRPC::CompletionQueueWorker<grpc::ServerCompletionQueue>;

        explicit CompletionQueueWorker(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : Base(std::move(queue))
        { }

        ~CompletionQueueWorker() override { deleteAllCallHandlers(); }

        CompletionQueueWorker(const CompletionQueueWorker&) = delete;
        CompletionQueueWorker& operator=(const CompletionQueueWorker&) = delete;


        // CallHandlers ------------------------------------------------------------------------------------------------
    public:
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

    private:
        using CallHandlerSet = std::unordered_set<Detail::AsyncCallHandlerBase*>;

        template<typename RequestFunc>
        auto newUnaryCallHandler(RequestFunc requestFunc,
            typename AsyncRequestTraits<RequestFunc>::ServiceType* service,
            std::function<void(grpc::ServerContext&, const typename AsyncRequestTraits<RequestFunc>::RequestType&,
                typename AsyncRequestTraits<RequestFunc>::ResponseType&)>
                processFunc)
        {
            auto handler =
                new Detail::AsyncUnaryCallHandler<RequestFunc>(queue(), service, requestFunc, std::move(processFunc));
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