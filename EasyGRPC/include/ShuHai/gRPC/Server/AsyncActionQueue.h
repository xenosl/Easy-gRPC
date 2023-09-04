#pragma once

#include "ShuHai/gRPC/Server/AsyncUnaryCallHandler.h"
#include "ShuHai/gRPC/Server/AsyncClientStreamCallHandler.h"
#include "ShuHai/gRPC/AsyncActionQueue.h"
#include "ShuHai/gRPC/CustomAsyncAction.h"

#include <grpcpp/alarm.h>

#include <unordered_set>
#include <mutex>
#include <atomic>

namespace ShuHai::gRPC::Server
{
    class AsyncActionQueue : public gRPC::AsyncActionQueue
    {
    public:
        explicit AsyncActionQueue(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : gRPC::AsyncActionQueue(std::move(queue))
        {
            _completionQueue = dynamic_cast<grpc::ServerCompletionQueue*>(gRPC::AsyncActionQueue::completionQueue());
        }

        ~AsyncActionQueue() override = default;

        void shutdown() override
        {
            shutdownCallHandlers();
            deleteAllCallHandlers();

            gRPC::AsyncActionQueue::shutdown();
        }

        /**
         * \brief The underlying grpc::ServerCompletionQueue that current instance wraps.
         */
        [[nodiscard]] grpc::ServerCompletionQueue* completionQueue() const { return _completionQueue; }

    private:
        grpc::ServerCompletionQueue* _completionQueue {};


        // Call Handlers -----------------------------------------------------------------------------------------------
    public:
        template<typename RequestFunc>
        EnableIfAnyRpcTypeMatch<void, RequestFunc, RpcType::UnaryCall> registerCallHandler(
            typename AsyncUnaryCallHandler<RequestFunc>::Service* service, RequestFunc requestFunc,
            typename AsyncUnaryCallHandler<RequestFunc>::HandleFunc handleFunc,
            asio::execution_context* handleFuncExecutionContext = nullptr)
        {
            if (!handleFunc)
                throw std::invalid_argument("Null handleFunc.");

            newCallHandler<AsyncUnaryCallHandler<RequestFunc>>(
                _completionQueue, service, requestFunc, std::move(handleFunc), handleFuncExecutionContext);
        }

        template<typename RequestFunc>
        EnableIfAnyRpcTypeMatch<void, RequestFunc, RpcType::ClientStream> registerCallHandler(
            typename AsyncClientStreamCallHandler<RequestFunc>::Service* service, RequestFunc requestFunc,
            typename AsyncClientStreamCallHandler<RequestFunc>::HandleFunc handleFunc)
        {
            if (!handleFunc)
                throw std::invalid_argument("Null handleFunc.");

            newCallHandler<AsyncClientStreamCallHandler<RequestFunc>>(
                _completionQueue, service, requestFunc, std::move(handleFunc));
        }

    private:
        using CallHandlerSet = std::unordered_set<AsyncCallHandlerBase*>;

        void shutdownCallHandlers()
        {
            for (auto h : _callHandlers)
                h->shutdown();
        }

        template<typename T, typename... Args>
        T* newCallHandler(Args&&... args)
        {
            auto handler = new T(std::forward<Args>(args)...);
            {
                std::lock_guard l(_callHandlersMutex);
                _callHandlers.emplace(handler);
            }
            return handler;
        }

        bool deleteCallHandler(AsyncCallHandlerBase* handler)
        {
            std::lock_guard l(_callHandlersMutex);

            auto it = _callHandlers.find(handler);
            if (it == _callHandlers.end())
                return false;
            doDeleteCallHandler(it);
            return true;
        }

        void deleteAllCallHandlers()
        {
            std::lock_guard l(_callHandlersMutex);

            auto it = _callHandlers.begin();
            while (it != _callHandlers.end())
                it = doDeleteCallHandler(it);
        }

        CallHandlerSet::iterator doDeleteCallHandler(CallHandlerSet::iterator it)
        {
            assert(it != _callHandlers.end());
            auto handler = *it;
            it = _callHandlers.erase(it);
            delete handler;
            return it;
        }

        std::mutex _callHandlersMutex;
        CallHandlerSet _callHandlers;
    };
}
