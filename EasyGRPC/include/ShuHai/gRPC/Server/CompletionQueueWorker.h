#pragma once

#include "ShuHai/gRPC/Server/AsyncUnaryCallHandler.h"
#include "ShuHai/gRPC/Server/AsyncClientStreamCallHandler.h"
#include "ShuHai/gRPC/CompletionQueueWorker.h"
#include "ShuHai/gRPC/CustomAsyncAction.h"

#include <grpcpp/alarm.h>

#include <unordered_set>
#include <atomic>

namespace ShuHai::gRPC::Server
{
    class CompletionQueueWorker : public gRPC::CompletionQueueWorker
    {
    public:
        explicit CompletionQueueWorker(std::unique_ptr<grpc::ServerCompletionQueue> queue)
            : gRPC::CompletionQueueWorker(std::move(queue))
        {
            _queue = dynamic_cast<grpc::ServerCompletionQueue*>(gRPC::CompletionQueueWorker::queue());
        }

        ~CompletionQueueWorker() override { new DeleteAllCallHandlersAction(this, _queue); }

        void shutdown() override
        {
            gRPC::CompletionQueueWorker::shutdown();
            shutdownCallHandlers();
        }

        [[nodiscard]] grpc::ServerCompletionQueue* queue() const { return _queue; }

    private:
        class Action : public CustomAsyncAction
        {
        public:
            Action(CompletionQueueWorker* owner, grpc::ServerCompletionQueue* cq)
                : _owner(owner)
            {
                this->trigger(cq);
            }

        protected:
            CompletionQueueWorker* const _owner;
        };

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

            new NewCallHandlerAction<AsyncUnaryCallHandler<RequestFunc>>(
                this, _queue, service, requestFunc, std::move(handleFunc));
        }

        template<typename RequestFunc>
        EnableIfAnyRpcTypeMatch<void, RequestFunc, RpcType::ClientStream> registerCallHandler(
            typename AsyncClientStreamCallHandler<RequestFunc>::Service* service, RequestFunc requestFunc,
            typename AsyncClientStreamCallHandler<RequestFunc>::HandleFunc handleFunc)
        {
            if (!handleFunc)
                throw std::invalid_argument("Null handleFunc.");

            new NewCallHandlerAction<AsyncClientStreamCallHandler<RequestFunc>>(
                this, _queue, service, requestFunc, std::move(handleFunc));
        }

    private:
        using CallHandlerSet = std::unordered_set<AsyncCallHandlerBase*>;

        template<typename Handler>
        class NewCallHandlerAction : public Action
        {
        public:
            static_assert(std::is_base_of_v<AsyncCallHandlerBase, Handler>);

            explicit NewCallHandlerAction(CompletionQueueWorker* owner, grpc::ServerCompletionQueue* cq,
                typename Handler::Service* service, typename Handler::RequestFunc requestFunc,
                typename Handler::HandleFunc handleFunc)
                : Action(owner, cq)
            {
                _handler.store(new Handler(cq, service, requestFunc, std::move(handleFunc)), std::memory_order_release);
            }

            void finalizeResult(bool ok) override
            {
                Handler* handler;
                while (!(handler = _handler.load(std::memory_order_acquire)))
                    continue;

                assert(handler);
                if (ok)
                    this->_owner->addHandler(handler);
                else
                    delete handler; // Canceled.
            }

        private:
            std::atomic<Handler*> _handler {};
        };

        class DeleteCallHandlerAction : public Action
        {
        public:
            DeleteCallHandlerAction(
                CompletionQueueWorker* owner, grpc::ServerCompletionQueue* cq, AsyncCallHandlerBase* handler)
                : Action(owner, cq)
            {
                assert(handler);
                _handler.store(handler, std::memory_order_release);
            }

            void finalizeResult(bool ok) override
            {
                assert(ok); // Cancellation of this action is not allowed.

                AsyncCallHandlerBase* handler;
                while (!(handler = _handler.load(std::memory_order_acquire)))
                    continue;

                assert(handler);
                this->_owner->deleteCallHandler(handler);
            }

        private:
            std::atomic<AsyncCallHandlerBase*> _handler;
        };

        class DeleteAllCallHandlersAction : public Action
        {
        public:
            explicit DeleteAllCallHandlersAction(CompletionQueueWorker* owner, grpc::ServerCompletionQueue* cq)
                : Action(owner, cq)
            { }

            void finalizeResult(bool ok) override
            {
                assert(ok); // Cancellation of this action is not allowed.
                this->_owner->deleteAllCallHandlers();
            }
        };

        void shutdownCallHandlers()
        {
            for (auto h : _callHandlers)
                h->shutdown();
        }

        void addHandler(AsyncCallHandlerBase* handler)
        {
            auto ret = _callHandlers.emplace(handler);
            assert(ret.second);
        }

        bool deleteCallHandler(AsyncCallHandlerBase* handler)
        {
            auto it = _callHandlers.find(handler);
            if (it == _callHandlers.end())
                return false;
            deleteCallHandler(it);
            return true;
        }

        void deleteAllCallHandlers()
        {
            auto it = _callHandlers.begin();
            while (it != _callHandlers.end())
                it = deleteCallHandler(it);
        }

        CallHandlerSet::iterator deleteCallHandler(CallHandlerSet::iterator it)
        {
            assert(it != _callHandlers.end());
            auto handler = *it;
            _callHandlers.erase(it);
            delete handler;
        }

        CallHandlerSet _callHandlers;
    };
}
