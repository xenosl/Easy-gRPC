#pragma once

#include "ShuHai/gRPC/Server/AsyncUnaryCallHandler.h"
#include "ShuHai/gRPC/CompletionQueueWorker.h"

#include <unordered_set>
#include <mutex>

namespace ShuHai::gRPC::Server
{
    class CompletionQueueWorker : public gRPC::CompletionQueueWorker
    {
    public:
        explicit CompletionQueueWorker(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : gRPC::CompletionQueueWorker(std::move(queue))
        {
            _queue = dynamic_cast<grpc::ServerCompletionQueue*>(this->queue());
        }

        ~CompletionQueueWorker() override
        {
            waitForShutdown();
            deleteCallHandlers();
        }

        void shutdown() override
        {
            gRPC::CompletionQueueWorker::shutdown();
            shutdownCallHandlers();
        }

    private:
        grpc::ServerCompletionQueue* _queue {};


        // Call Handlers -----------------------------------------------------------------------------------------------
    public:
        template<typename RequestFunc>
        EnableIfAnyRpcTypeMatch<void, RequestFunc, RpcType::UnaryCall> registerCallHandler(
            typename AsyncUnaryCallHandler<RequestFunc>::Service* service, RequestFunc requestFunc,
            typename AsyncUnaryCallHandler<RequestFunc>::HandleFunc handleFunc)
        {
            if (!handleFunc)
                throw std::invalid_argument("Null handleFunc.");

            auto handler = new AsyncUnaryCallHandler<RequestFunc>(_queue, service, requestFunc, std::move(handleFunc));
            addCallHandler(handler);
        }

    private:
        using CallHandlerSet = std::unordered_set<AsyncCallHandlerBase*>;

        void shutdownCallHandlers()
        {
            std::lock_guard l(_callHandlersMutex);
            for (auto h : _callHandlers)
                h->shutdown();
        }

        void deleteCallHandlers()
        {
            auto callHandlers = takeCallHandlers();
            for (auto h : callHandlers)
                delete h;
            callHandlers.clear();
        }

        void addCallHandler(AsyncCallHandlerBase* handler)
        {
            std::lock_guard l(_callHandlersMutex);
            _callHandlers.emplace(handler);
        }

        CallHandlerSet takeCallHandlers()
        {
            std::lock_guard l(_callHandlersMutex);
            return std::move(_callHandlers);
        }

        // TODO: Use lock-free container instead.
        std::mutex _callHandlersMutex;
        CallHandlerSet _callHandlers;
    };
}
