#pragma once

#include "ShuHai/gRPC/Client/AsyncUnaryCall.h"
#include "ShuHai/gRPC/CompletionQueueWorker.h"

#include <grpcpp/grpcpp.h>

#include <thread>
#include <future>
#include <unordered_set>
#include <tuple>

namespace ShuHai::gRPC::Client
{
    template<typename... Stubs>
    class AsyncClient
    {
    public:
        explicit AsyncClient(const std::string& targetEndpoint,
            std::shared_ptr<grpc::ChannelCredentials> credentials = grpc::InsecureChannelCredentials(),
            const grpc::ChannelArguments& channelArguments = {})
        {
            _channel = grpc::CreateCustomChannel(targetEndpoint, credentials, channelArguments);
            initStubs();
            initCompletionQueue();
        }

        ~AsyncClient() { deinitCompletionQueue(); }

    private:
        std::shared_ptr<grpc::Channel> _channel;


        // Calls -------------------------------------------------------------------------------------------------------
    public:
        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Async<RpcName>.
         *  Get result by function AsyncUnaryCall<CallFunc>::getResponseFuture() of the returned call instance.
         * \param asyncCall The function address of Stub::Async<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \return The call instance.
         */
        template<typename CallFunc>
        EnableIfRpcTypeMatch<CallFunc, RpcType::NORMAL_RPC, std::shared_ptr<AsyncUnaryCall<CallFunc>>> call(
            CallFunc asyncCall, const RequestTypeOf<CallFunc>& request,
            std::unique_ptr<grpc::ClientContext> context = nullptr)
        {
            using Call = AsyncUnaryCall<CallFunc>;
            auto call = std::make_shared<Call>(stub<typename Call::Stub>(), asyncCall, request, _cqWorker->queue(),
                std::move(context), nullptr, [this](std::shared_ptr<Call> c) { onCallDead(c); });
            addStreamingCall(call);
            return call;
        }

        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Async<RpcName>.
         *  Get notification and result by a callback function.
         * \param asyncCall The function address of Stub::Async<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \param callback The callback function for rpc result notification.
         * \return The call instance.
         */
        template<typename CallFunc>
        EnableIfRpcTypeMatch<CallFunc, RpcType::NORMAL_RPC, std::shared_ptr<AsyncUnaryCall<CallFunc>>> call(
            CallFunc asyncCall, const RequestTypeOf<CallFunc>& request,
            typename AsyncUnaryCall<CallFunc>::ResponseCallback callback,
            std::unique_ptr<grpc::ClientContext> context = nullptr)
        {
            using Call = AsyncUnaryCall<CallFunc>;
            auto call = std::make_shared<Call>(stub<typename Call::Stub>(), asyncCall, request, _cqWorker->queue(),
                std::move(context), std::move(callback), [this](std::shared_ptr<Call> c) { onCallDead(c); });
            addStreamingCall(call);
            return call;
        }

    private:
        using CallPtr = std::shared_ptr<AsyncCall>;

        void onCallDead(CallPtr call) { removeStreamingCall(call); }

        void addStreamingCall(CallPtr call)
        {
            std::lock_guard l(_streamingCallsMutex);
            _streamingCalls.emplace(call);
        }

        void removeStreamingCall(CallPtr call)
        {
            std::lock_guard l(_streamingCallsMutex);
            _streamingCalls.erase(call);
        }

        // TODO: Use lock-free container instead.
        std::mutex _streamingCallsMutex;
        std::unordered_set<CallPtr> _streamingCalls;

        // Queue Worker ------------------------------------------------------------------------------------------------
    private:
        void initCompletionQueue()
        {
            _cqWorker = std::make_unique<CompletionQueueWorker>(std::make_unique<grpc::CompletionQueue>());

            _cqThread = std::thread(
                [this]()
                {
                    while (true)
                    {
                        if (!_cqWorker->poll())
                            break;
                    }
                });
        }

        void deinitCompletionQueue()
        {
            _cqWorker->shutdown();

            _cqThread.join();

            _cqWorker = nullptr;
        }

        std::unique_ptr<CompletionQueueWorker> _cqWorker;
        std::thread _cqThread;


        // Stubs -------------------------------------------------------------------------------------------------------
    public:
        using StubIndices = std::index_sequence_for<Stubs...>;
        static constexpr size_t StubCount = StubIndices::size();
        static_assert(StubCount > 0);

        template<typename Stub>
        static constexpr bool isSupportedStubType()
        {
            return ((std::is_same_v<Stub, Stubs>) || ...);
        }

        template<typename Stub>
        [[nodiscard]] Stub* stub() const
        {
            return std::get<indexOfStub<Stub>()>(_stubs).get();
        }

    private:
        template<typename Stub>
        static constexpr size_t indexOfStub()
        {
            static_assert(isSupportedStubType<Stub>());
            return indexOfStubImpl<0, Stub>();
        }

        template<size_t I, typename Stub>
        static constexpr size_t indexOfStubImpl()
        {
            static_assert(I < StubCount);
            if constexpr (std::is_same_v<Stub, typename std::tuple_element_t<I, StubTuple>::element_type>)
                return I;
            else
                return indexOfStubImpl<I + 1, Stub>();
        }

        void initStubs() { _stubs = std::make_tuple(std::make_unique<Stubs>(_channel)...); }

        using StubTuple = std::tuple<std::unique_ptr<Stubs>...>;
        StubTuple _stubs;
    };
}
